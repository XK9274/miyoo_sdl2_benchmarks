#include "common/render3d/pipeline.h"

#include <math.h>
#include <stdlib.h>

/* Post-clip vertex: view-space position (depth key + projection) and
 * texcoord. Color is per-triangle, not per-vertex -- flat shading computes
 * one shared color before clipping. */
typedef struct {
    Vec3 view_pos;
    Vec2 texcoord;
} ClipVertex;

typedef struct {
    SDL_Vertex verts[3];
    SDL_Texture *texture;
    float depth_key; /* view-space Z average; more negative = farther (camera looks down -Z) */
} PipelineTriangle;

struct Render3DScratch {
    PipelineTriangle *triangles;
    int count;
    int capacity;

    /* Populated from `triangles` after sorting, in sorted order, only when
     * fill (non-wireframe) batching needs a contiguous run of vertices to
     * hand SDL_RenderGeometry in one call. */
    SDL_Vertex *flat_verts;      /* capacity * 3 */
    SDL_Texture **flat_textures; /* capacity */
};

Render3DScratch *render3d_scratch_create(void)
{
    return (Render3DScratch *)calloc(1, sizeof(Render3DScratch));
}

void render3d_scratch_destroy(Render3DScratch *scratch)
{
    if (!scratch) {
        return;
    }
    free(scratch->triangles);
    free(scratch->flat_verts);
    free(scratch->flat_textures);
    free(scratch);
}

static SDL_bool pipeline_ensure_capacity(Render3DScratch *scratch, int needed)
{
    if (scratch->capacity >= needed) {
        return SDL_TRUE;
    }

    PipelineTriangle *triangles = (PipelineTriangle *)realloc(scratch->triangles, sizeof(PipelineTriangle) * (size_t)needed);
    SDL_Vertex *flat_verts = (SDL_Vertex *)realloc(scratch->flat_verts, sizeof(SDL_Vertex) * (size_t)needed * 3);
    SDL_Texture **flat_textures = (SDL_Texture **)realloc(scratch->flat_textures, sizeof(SDL_Texture *) * (size_t)needed);

    if (triangles) scratch->triangles = triangles;
    if (flat_verts) scratch->flat_verts = flat_verts;
    if (flat_textures) scratch->flat_textures = flat_textures;

    if (!triangles || !flat_verts || !flat_textures) {
        return SDL_FALSE;
    }

    scratch->capacity = needed;
    return SDL_TRUE;
}

static Uint8 pipeline_clamp_channel(float v)
{
    if (v < 0.0f) v = 0.0f;
    if (v > 1.0f) v = 1.0f;
    return (Uint8)(v * 255.0f + 0.5f);
}

static Vec3 pipeline_transform_position(const Mat4 *view, Vec3 p)
{
    /* view is always affine (bottom row 0,0,0,1), so w is always 1. */
    const Vec4 r = bench_mat4_transform_point(view, p);
    Vec3 out = {r.x, r.y, r.z};
    return out;
}

static SDL_bool pipeline_clip_vertex_inside(const ClipVertex *v, float near_plane)
{
    return v->view_pos.z <= -near_plane;
}

static ClipVertex pipeline_clip_lerp(const ClipVertex *a, const ClipVertex *b, float near_plane)
{
    const float denom = b->view_pos.z - a->view_pos.z;
    float t = 0.5f;
    if (fabsf(denom) > 1e-6f) {
        t = (-near_plane - a->view_pos.z) / denom;
    }

    ClipVertex r;
    r.view_pos.x = a->view_pos.x + (b->view_pos.x - a->view_pos.x) * t;
    r.view_pos.y = a->view_pos.y + (b->view_pos.y - a->view_pos.y) * t;
    r.view_pos.z = -near_plane;
    r.texcoord.u = a->texcoord.u + (b->texcoord.u - a->texcoord.u) * t;
    r.texcoord.v = a->texcoord.v + (b->texcoord.v - a->texcoord.v) * t;
    return r;
}

/* Sutherland-Hodgman clip of one triangle against the single near plane
 * (view.z <= -near_plane is "inside"). Returns 0 (fully clipped), 3, or 4
 * vertices in out (a 4-vertex result fans into 2 triangles: (0,1,2),(0,2,3)). */
static int pipeline_clip_near(const ClipVertex in[3], ClipVertex out[4], float near_plane)
{
    int out_count = 0;
    for (int i = 0; i < 3; i++) {
        const ClipVertex *curr = &in[i];
        const ClipVertex *prev = &in[(i + 2) % 3];
        const SDL_bool curr_in = pipeline_clip_vertex_inside(curr, near_plane);
        const SDL_bool prev_in = pipeline_clip_vertex_inside(prev, near_plane);

        if (curr_in) {
            if (!prev_in) {
                out[out_count++] = pipeline_clip_lerp(prev, curr, near_plane);
            }
            out[out_count++] = *curr;
        } else if (prev_in) {
            out[out_count++] = pipeline_clip_lerp(prev, curr, near_plane);
        }
    }
    return out_count;
}

static SDL_Vertex pipeline_project_vertex(const ClipVertex *cv, const Mat4 *projection,
                                          const Render3DFrameParams *params, SDL_Color color)
{
    const Vec4 clip = bench_mat4_transform_point(projection, cv->view_pos);
    const float inv_w = (fabsf(clip.w) > 1e-6f) ? (1.0f / clip.w) : 1.0f;
    const float ndc_x = clip.x * inv_w;
    const float ndc_y = clip.y * inv_w;

    SDL_Vertex v;
    /* NDC (-1..1) maps into the content sub-rectangle (viewport_x/y/w/h),
     * not necessarily the whole screen, so the model centers below an
     * overlay/HUD rather than behind it. */
    v.position.x = (float)params->viewport_x + (ndc_x * 0.5f + 0.5f) * (float)params->viewport_width;
    /* NDC +Y is up, SDL screen +Y is down -- flipped here (separate from the
     * earlier OBJ-to-SDL texture V flip). */
    v.position.y = (float)params->viewport_y + (1.0f - (ndc_y * 0.5f + 0.5f)) * (float)params->viewport_height;
    v.color = color;
    v.tex_coord.x = cv->texcoord.u;
    v.tex_coord.y = cv->texcoord.v;
    return v;
}

static int pipeline_compare_depth(const void *a, const void *b)
{
    const PipelineTriangle *ta = (const PipelineTriangle *)a;
    const PipelineTriangle *tb = (const PipelineTriangle *)b;
    if (ta->depth_key < tb->depth_key) return -1;
    if (ta->depth_key > tb->depth_key) return 1;
    return 0;
}

void render3d_draw_mesh(SDL_Renderer *renderer,
                        const Mesh *mesh,
                        SDL_Texture *const *material_textures,
                        const Render3DFrameParams *params,
                        Render3DScratch *scratch,
                        BenchMetrics *metrics)
{
    if (mesh->triangle_count == 0) {
        return;
    }

    /* Near-plane clipping can at most double a triangle. */
    if (!pipeline_ensure_capacity(scratch, mesh->triangle_count * 2)) {
        return;
    }
    scratch->count = 0;

    const Vec3 to_light = bench_vec3_scale(bench_vec3_normalize(params->light_direction), -1.0f);

    for (int tri = 0; tri < mesh->triangle_count; tri++) {
        const MeshVertex *mv0 = &mesh->vertices[tri * 3 + 0];
        const MeshVertex *mv1 = &mesh->vertices[tri * 3 + 1];
        const MeshVertex *mv2 = &mesh->vertices[tri * 3 + 2];

        const Vec3 view_pos0 = pipeline_transform_position(&params->view, mv0->position);
        const Vec3 view_pos1 = pipeline_transform_position(&params->view, mv1->position);
        const Vec3 view_pos2 = pipeline_transform_position(&params->view, mv2->position);

        /* Winding-derived normal from view-space positions, independent of
         * authored vertex normals -- front-facing (CCW) triangles satisfy
         * dot(normal, v0) < 0. */
        const Vec3 cull_normal = bench_vec3_cross(
            bench_vec3_sub(view_pos1, view_pos0),
            bench_vec3_sub(view_pos2, view_pos0));
        if (bench_vec3_dot(cull_normal, view_pos0) >= 0.0f) {
            continue; /* back-facing */
        }

        Vec3 shading_normal;
        if (mv0->has_normal && mv1->has_normal && mv2->has_normal) {
            shading_normal = bench_vec3_normalize(bench_vec3_add(
                bench_vec3_add(mv0->normal, mv1->normal), mv2->normal));
        } else {
            /* Missing-normal fallback: geometric normal from object-space
             * positions (object space == world space; the model transform
             * is always identity in this suite). */
            shading_normal = bench_vec3_normalize(bench_vec3_cross(
                bench_vec3_sub(mv1->position, mv0->position),
                bench_vec3_sub(mv2->position, mv0->position)));
        }

        const float brightness = fmaxf(bench_vec3_dot(shading_normal, to_light), params->ambient_floor);

        const int material_id = mesh->triangle_material_ids[tri];
        const MeshMaterial *material = &mesh->materials[material_id];
        SDL_Texture *texture = material_textures ? material_textures[material_id] : NULL;

        const SDL_Color color = {
            pipeline_clamp_channel(material->diffuse_color[0] * brightness),
            pipeline_clamp_channel(material->diffuse_color[1] * brightness),
            pipeline_clamp_channel(material->diffuse_color[2] * brightness),
            pipeline_clamp_channel(material->opacity)
        };

        const ClipVertex clip_in[3] = {
            {view_pos0, mv0->texcoord},
            {view_pos1, mv1->texcoord},
            {view_pos2, mv2->texcoord}
        };
        ClipVertex clip_out[4];
        const int clip_out_count = pipeline_clip_near(clip_in, clip_out, params->near_plane);
        if (clip_out_count < 3) {
            continue; /* fully behind the near plane */
        }

        /* Fan-triangulate the (3 or 4 vertex) clipped polygon. */
        for (int t = 0; t + 2 < clip_out_count; t++) {
            const ClipVertex *a = &clip_out[0];
            const ClipVertex *b = &clip_out[t + 1];
            const ClipVertex *c = &clip_out[t + 2];

            PipelineTriangle *dst = &scratch->triangles[scratch->count++];
            dst->verts[0] = pipeline_project_vertex(a, &params->projection, params, color);
            dst->verts[1] = pipeline_project_vertex(b, &params->projection, params, color);
            dst->verts[2] = pipeline_project_vertex(c, &params->projection, params, color);
            dst->texture = texture;
            dst->depth_key = (a->view_pos.z + b->view_pos.z + c->view_pos.z) / 3.0f;
        }
    }

    if (scratch->count == 0) {
        return;
    }

    /* Global back-to-front sort, kept intact afterward -- depth correctness
     * takes priority over grouping by texture for batching. */
    qsort(scratch->triangles, (size_t)scratch->count, sizeof(PipelineTriangle), pipeline_compare_depth);

    if (params->wireframe) {
        for (int i = 0; i < scratch->count; i++) {
            const PipelineTriangle *t = &scratch->triangles[i];
            SDL_FPoint pts[4] = {t->verts[0].position, t->verts[1].position, t->verts[2].position, t->verts[0].position};
            SDL_SetRenderDrawColor(renderer, t->verts[0].color.r, t->verts[0].color.g, t->verts[0].color.b, t->verts[0].color.a);
            SDL_RenderDrawLinesF(renderer, pts, 4);
            metrics->draw_calls++;
            metrics->vertices_rendered += 3;
            metrics->triangles_rendered += 1;
        }
        return;
    }

    /* Copy into a contiguous, sorted-order buffer so a run of consecutive
     * same-texture triangles can be handed to SDL_RenderGeometry in one
     * call without disturbing the sort. */
    for (int i = 0; i < scratch->count; i++) {
        scratch->flat_verts[i * 3 + 0] = scratch->triangles[i].verts[0];
        scratch->flat_verts[i * 3 + 1] = scratch->triangles[i].verts[1];
        scratch->flat_verts[i * 3 + 2] = scratch->triangles[i].verts[2];
        scratch->flat_textures[i] = scratch->triangles[i].texture;
    }

    int run_start = 0;
    while (run_start < scratch->count) {
        SDL_Texture *run_texture = scratch->flat_textures[run_start];
        int run_end = run_start + 1;
        while (run_end < scratch->count && scratch->flat_textures[run_end] == run_texture) {
            run_end++;
        }
        const int run_len = run_end - run_start;

        SDL_RenderGeometry(renderer, run_texture, &scratch->flat_verts[run_start * 3], run_len * 3, NULL, 0);

        metrics->draw_calls++;
        metrics->vertices_rendered += (Uint64)run_len * 3;
        metrics->triangles_rendered += (Uint64)run_len;
        metrics->texture_switches++;

        run_start = run_end;
    }
}
