/*!
 * @file
 * @brief This file contains functions for model rendering
 *
 * @author Tomáš Milet, imilet@fit.vutbr.cz
 */
#include <student/drawModel.hpp>
#include <student/gpu.hpp>


void draw_node(GPUContext &ctx, const Node &node, const Model &model, const glm::mat4 &proj_view, glm::mat4 ongoing_mat)
{ 
    ongoing_mat = ongoing_mat * node.modelMatrix;
    if (node.mesh >= 0)
    {
        Texture empty_texture;
        Mesh mesh = model.meshes[node.mesh];

        ctx.prg.uniforms.uniform[0].m4 = proj_view;
        ctx.prg.uniforms.uniform[1].m4 = ongoing_mat;
        ctx.prg.uniforms.uniform[2].m4 = glm::transpose(glm::inverse(ongoing_mat));
        ctx.prg.uniforms.uniform[5].v4 = mesh.diffuseColor;

        ctx.vao.indexBuffer = mesh.indices;
        ctx.vao.indexType = mesh.indexType;

        ctx.vao.vertexAttrib[0] = mesh.position;
        ctx.prg.vs2fs[0] = AttributeType::VEC3;
        ctx.vao.vertexAttrib[1] = mesh.normal;
        ctx.prg.vs2fs[1] = AttributeType::VEC3;
        ctx.vao.vertexAttrib[2] = mesh.texCoord;
        ctx.prg.vs2fs[2] = AttributeType::VEC2;

        if (mesh.diffuseTexture >= 0)
        {
            ctx.prg.uniforms.textures[0] = model.textures[mesh.diffuseTexture];
            ctx.prg.uniforms.uniform[6].v1 = 1.0f;
        }
        else
        {
            ctx.prg.uniforms.textures[0] = empty_texture;
            ctx.prg.uniforms.uniform[6].v1 = 0.0f;
        }

        ctx.prg.fragmentShader = drawModel_fragmentShader;
        ctx.prg.vertexShader = drawModel_vertexShader;

        drawTriangles(ctx, mesh.nofIndices);    
    }
    
    for(const Node &child: node.children)
    {
        draw_node(ctx, child, model, proj_view, ongoing_mat);
    }
}

/**
 * @brief This function renders a model
 *
 * @param ctx GPUContext
 * @param model model structure
 * @param proj projection matrix
 * @param view view matrix
 * @param light light position
 * @param camera camera position (unused)
 */
//! [drawModel]
void drawModel(GPUContext&ctx,Model const&model,glm::mat4 const&proj,glm::mat4 const&view,glm::vec3 const&light,glm::vec3 const&camera)
{
    ctx.prg.uniforms.uniform[3].v3 = light;
    ctx.prg.uniforms.uniform[4].v3 = camera;

    glm::mat4 proj_view = proj * view;
    glm::mat4 start_mat = glm::mat4(1.f);

    for (Node const &node: model.roots)
    {
        draw_node(ctx, node, model, proj_view, start_mat);
    }
}
//! [drawModel]

/**
 * @brief This function represents vertex shader of texture rendering method.
 *
 * @param outVertex output vertex
 * @param inVertex input vertex
 * @param uniforms uniform variables
 */
//! [drawModel_vs]
void drawModel_vertexShader(OutVertex&outVertex,InVertex const&inVertex,Uniforms const&uniforms)
{
    outVertex.attributes[0].v4 = uniforms.uniform[1].m4 * inVertex.attributes[0].v4;
    outVertex.attributes[1].v4 = uniforms.uniform[2].m4 * inVertex.attributes[1].v4;
    outVertex.attributes[2] = inVertex.attributes[2]; 

    outVertex.gl_Position = uniforms.uniform[0].m4 * outVertex.attributes[0].v4;
}
//! [drawModel_vs]

/**
 * @brief This functionrepresents fragment shader of texture rendering method.
 *
 * @param outFragment output fragment
 * @param inFragment input fragment
 * @param uniforms uniform variables
 */
//! [drawModel_fs]
void drawModel_fragmentShader(OutFragment&outFragment,InFragment const&inFragment,Uniforms const&uniforms)
{
    float dF = glm::clamp(glm::dot(glm::normalize(uniforms.uniform[3].v3 - inFragment.attributes[0].v3), 
                                   glm::normalize(inFragment.attributes[1].v3)), 0.0f, 1.0f);
    
    dF += 0.2f;
    if (uniforms.uniform[6].v1 > 0.0f)
    {
        glm::vec4 color = read_texture(uniforms.textures[0], inFragment.attributes[2].v2);
        outFragment.gl_FragColor = glm::vec4(color.r * dF,
                                             color.g * dF,
                                             color.b * dF,
                                             color.a);
    }
    else
    {
        outFragment.gl_FragColor = glm::vec4(uniforms.uniform[5].v4.r * dF,
                                             uniforms.uniform[5].v4.g * dF,
                                             uniforms.uniform[5].v4.b * dF,
                                             uniforms.uniform[5].v4.a);
    } 
}
//! [drawModel_fs]
