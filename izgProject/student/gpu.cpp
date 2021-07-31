/*!
 * @file
 * @brief This file contains implementation of gpu
 *
 * @author Tomáš Milet, imilet@fit.vutbr.cz
 */

#include <student/gpu.hpp>
#include <thread>

const uint32_t THREAD_COUNT{16};

inline uint32_t get_vertex_id(const GPUContext &ctx, uint32_t vertex_number)
{
    if (ctx.vao.indexBuffer != nullptr)
    {
        if (ctx.vao.indexType == IndexType::UINT8)
        {
            return ((uint8_t *)(ctx.vao.indexBuffer))[vertex_number];
        }
        else if (ctx.vao.indexType == IndexType::UINT16)
        {
            return ((uint16_t *)(ctx.vao.indexBuffer))[vertex_number];
        }
        else if (ctx.vao.indexType == IndexType::UINT32)
        {
            return ((uint32_t *)(ctx.vao.indexBuffer))[vertex_number];
        }
    }

    return vertex_number;
}

inline void get_vertex_attributes(const GPUContext &ctx, InVertex &in_vertex)
{
    for (uint8_t i = 0; i < maxAttributes; i++)
    {
        switch (ctx.vao.vertexAttrib[i].type)
        {
            case AttributeType::FLOAT:
                in_vertex.attributes[i].v1 = ((float *)(((uint8_t *)(ctx.vao.vertexAttrib[i].bufferData)) + 
                                                           ctx.vao.vertexAttrib[i].offset +
                                                           ctx.vao.vertexAttrib[i].stride * in_vertex.gl_VertexID))[0];
                break;
            
            case AttributeType::VEC2:
                in_vertex.attributes[i].v2 = ((glm::vec2 *)(((uint8_t *)(ctx.vao.vertexAttrib[i].bufferData)) + 
                                                           ctx.vao.vertexAttrib[i].offset +
                                                           ctx.vao.vertexAttrib[i].stride * in_vertex.gl_VertexID))[0];
                break;
            
            case AttributeType::VEC3:
                in_vertex.attributes[i].v3 = ((glm::vec3 *)(((uint8_t *)(ctx.vao.vertexAttrib[i].bufferData)) + 
                                                           ctx.vao.vertexAttrib[i].offset +
                                                           ctx.vao.vertexAttrib[i].stride * in_vertex.gl_VertexID))[0];
                break;

            case AttributeType::VEC4:
                in_vertex.attributes[i].v4 = ((glm::vec4 *)(((uint8_t *)(ctx.vao.vertexAttrib[i].bufferData)) + 
                                                           ctx.vao.vertexAttrib[i].offset +
                                                           ctx.vao.vertexAttrib[i].stride * in_vertex.gl_VertexID))[0];
                break;

            default:
                break;
        }
    }
}

inline void viewport_transofrmation(OutVertex &out_vertex, uint32_t width, uint32_t height)
{
    // perspective division
    out_vertex.gl_Position.x = out_vertex.gl_Position.x / out_vertex.gl_Position.w;
    out_vertex.gl_Position.y = out_vertex.gl_Position.y / out_vertex.gl_Position.w;
    out_vertex.gl_Position.z = out_vertex.gl_Position.z / out_vertex.gl_Position.w;

    out_vertex.gl_Position.x = (out_vertex.gl_Position.x + 1.0f) * width * 0.5f;
    out_vertex.gl_Position.y = (out_vertex.gl_Position.y + 1.0f) * height * 0.5f;
}

inline void clamp_color(OutFragment &out_fragment)
{
    out_fragment.gl_FragColor.r = out_fragment.gl_FragColor.r >= 0.0f ? 
                                        (out_fragment.gl_FragColor.r <= 1.0f ? 
                                            out_fragment.gl_FragColor.r : 1.0f) 
                                    : 0.0f;

    out_fragment.gl_FragColor.g = out_fragment.gl_FragColor.g >= 0.0f ? 
                                        (out_fragment.gl_FragColor.g <= 1.0f ? 
                                            out_fragment.gl_FragColor.g : 1.0f) 
                                    : 0.0f;
    
    out_fragment.gl_FragColor.b = out_fragment.gl_FragColor.b >= 0.0f ? 
                                        (out_fragment.gl_FragColor.b <= 1.0f ? 
                                            out_fragment.gl_FragColor.b : 1.0f) 
                                    : 0.0f;
}

inline void per_fragment_operations(const Frame &frame, const OutFragment &out_fragment, float depth, uint32_t x, uint32_t y)
{
    size_t offset = y * frame.width + x;

    std::cerr << depth << "  " << frame.depth[offset] << std::endl;
    if (frame.depth[offset] > depth)
    {
        size_t R_offset_color = offset << 2;
        size_t G_offset_color = R_offset_color + 1;
        size_t B_offset_color = R_offset_color + 2;
        float rev_a = 1.0f - out_fragment.gl_FragColor.a;
        float alpha = 255 * out_fragment.gl_FragColor.a;

        frame.color[R_offset_color] = frame.color[R_offset_color] * rev_a +
                                      out_fragment.gl_FragColor.r * alpha;
        frame.color[G_offset_color] = frame.color[G_offset_color] * rev_a +
                                      out_fragment.gl_FragColor.g * alpha;
        frame.color[B_offset_color] = frame.color[B_offset_color] * rev_a +
                                      out_fragment.gl_FragColor.b * alpha;

        if (out_fragment.gl_FragColor.a > 0.5f)
        {
            frame.depth[offset] = depth;
        }
    }
}

inline void clipping(OutVertex &current, OutVertex &second, OutVertex &clipped)
{
    float t = (-current.gl_Position.z - current.gl_Position.w) / 
              (second.gl_Position.w - current.gl_Position.w + second.gl_Position.z - current.gl_Position.z);
    
    clipped.gl_Position = current.gl_Position + t * (second.gl_Position - current.gl_Position);
}

void rasterize_triangle(const OutVertex &X, const OutVertex &Y, const OutVertex &Z, const GPUContext &ctx)
{
    int x_min, x_max, y_min, y_max;

    x_min = std::max((int)std::min(std::min(X.gl_Position.x, Y.gl_Position.x), Z.gl_Position.x), 0);
    y_min = std::max((int)std::min(std::min(X.gl_Position.y, Y.gl_Position.y), Z.gl_Position.y), 0);
    
    x_max = std::min((int)std::max(std::max(X.gl_Position.x, Y.gl_Position.x), Z.gl_Position.x), (int)ctx.frame.width - 1);
    y_max = std::min((int)std::max(std::max(X.gl_Position.y, Y.gl_Position.y), Z.gl_Position.y), (int)ctx.frame.height - 1);

    float A0, B0;
    A0 = Z.gl_Position.y - Y.gl_Position.y;
    B0 = Z.gl_Position.x - Y.gl_Position.x;

    float A1, B1;
    A1 = X.gl_Position.y - Z.gl_Position.y;
    B1 = X.gl_Position.x - Z.gl_Position.x;

    float A2, B2;
    A2 = Y.gl_Position.y - X.gl_Position.y;
    B2 = Y.gl_Position.x - X.gl_Position.x;
    
    float S, invS, S0, S1, S2;
    S = B0 * A2 - A0 * B2;
    if (S == 0.0f)
    {
        return;
    }
    invS = 1.0f / S;

    float s;
    float l0, l1, l2;
    float l03D, l13D, l23D;
    float l0h0, l1h1, l2h2;

    InFragment in_fragment;
    OutFragment out_fragment;
    bool inside = true;

    for (int y = y_min; y <= y_max; y++)
    {
        inside = true;
        in_fragment.gl_FragCoord.y = (float)y + 0.5;
        for (int x = x_min; x <= x_max; x++)
        {   
            in_fragment.gl_FragCoord.x = (float)x + 0.5;
            S0 = (in_fragment.gl_FragCoord.x - Y.gl_Position.x) * A0 - B0 * (in_fragment.gl_FragCoord.y - Y.gl_Position.y);
            S1 = (in_fragment.gl_FragCoord.x - Z.gl_Position.x) * A1 - B1 * (in_fragment.gl_FragCoord.y - Z.gl_Position.y);
            S2 = (in_fragment.gl_FragCoord.x - X.gl_Position.x) * A2 - B2 * (in_fragment.gl_FragCoord.y - X.gl_Position.y); 
            
            if (S0 <= 0.0f && S1 <= 0.0f && S2 <= 0.0f)
            {   
                inside = false;
                l0 = S0 * invS;
                l1 = S1 * invS;
                l2 = S2 * invS;
                in_fragment.gl_FragCoord.z = X.gl_Position.z * l0 + Y.gl_Position.z * l1 + Z.gl_Position.z * l2;

                l0h0 = l0 / X.gl_Position.w;
                l1h1 = l1 / Y.gl_Position.w;
                l2h2 = l2 / Z.gl_Position.w;
                s = l0h0 + l1h1 + l2h2;
                l03D = l0h0 / s;
                l13D = l1h1 / s;
                l23D = l2h2 / s;
                
                for (uint32_t i = 0; i < maxAttributes; i++)
                {
                    switch (ctx.prg.vs2fs[i])
                    {
                        case AttributeType::FLOAT:
                            in_fragment.attributes[i].v1 = X.attributes[i].v1 * l03D + 
                                                           Y.attributes[i].v1 * l13D + 
                                                           Z.attributes[i].v1 * l23D;
                            break;

                        case AttributeType::VEC2:
                            in_fragment.attributes[i].v2 = X.attributes[i].v2 * l03D + 
                                                           Y.attributes[i].v2 * l13D + 
                                                           Z.attributes[i].v2 * l23D;
                            break;

                        case AttributeType::VEC3:
                            in_fragment.attributes[i].v3 = X.attributes[i].v3 * l03D + 
                                                           Y.attributes[i].v3 * l13D + 
                                                           Z.attributes[i].v3 * l23D;
                            break;

                        case AttributeType::VEC4:
                            in_fragment.attributes[i].v4 = X.attributes[i].v4 * l03D + 
                                                           Y.attributes[i].v4 * l13D + 
                                                           Z.attributes[i].v4 * l23D;
                            break;
                        default:
                            break;
                    }
                }

                ctx.prg.fragmentShader(out_fragment, in_fragment, ctx.prg.uniforms);
                clamp_color(out_fragment);
                per_fragment_operations(ctx.frame, out_fragment, in_fragment.gl_FragCoord.z, x, y);
            }
            else if (!inside)
            {
                break;
            }
        }
    }
}

void run_thread_rasterization(GPUContext &ctx, OutVertex X, OutVertex Y, OutVertex Z)
{
    OutVertex clipped;
    if (X.gl_Position.z >= -X.gl_Position.w && 
        Y.gl_Position.z >= -Y.gl_Position.w && 
        Z.gl_Position.z >= -Z.gl_Position.w)
    {
        viewport_transofrmation(X, ctx.frame.width, ctx.frame.height);
        viewport_transofrmation(Y, ctx.frame.width, ctx.frame.height);
        viewport_transofrmation(Z, ctx.frame.width, ctx.frame.height);
        rasterize_triangle(X, Y, Z, ctx);
    }
    else if (X.gl_Position.z < -X.gl_Position.w && 
             Y.gl_Position.z >= -Y.gl_Position.w && 
             Z.gl_Position.z >= -Z.gl_Position.w)
    {
        clipped = X;
        clipping(X, Z, clipped);
        clipping(X, Y, X);
        
        viewport_transofrmation(X, ctx.frame.width, ctx.frame.height);
        viewport_transofrmation(Y, ctx.frame.width, ctx.frame.height);
        viewport_transofrmation(Z, ctx.frame.width, ctx.frame.height);
        viewport_transofrmation(clipped, ctx.frame.width, ctx.frame.height);

        rasterize_triangle(X, Y, Z, ctx);
        rasterize_triangle(clipped, Y, X, ctx);
    }
    else if (X.gl_Position.z >= -X.gl_Position.w && 
             Y.gl_Position.z < -Y.gl_Position.w && 
             Z.gl_Position.z >= -Z.gl_Position.w)
    {
        clipped = Y;
        clipping(Y, X, clipped);
        clipping(Y, Z, Y);
    
        viewport_transofrmation(X, ctx.frame.width, ctx.frame.height);
        viewport_transofrmation(Y, ctx.frame.width, ctx.frame.height);
        viewport_transofrmation(Z, ctx.frame.width, ctx.frame.height);
        viewport_transofrmation(clipped, ctx.frame.width, ctx.frame.height);
        rasterize_triangle(X, Y, Z, ctx);
        rasterize_triangle(Z, Y, clipped, ctx);
    }
    else if (X.gl_Position.z >= -X.gl_Position.w && 
             Y.gl_Position.z >= -Y.gl_Position.w && 
             Z.gl_Position.z < -Z.gl_Position.w)
    {
        clipped = Z;
        clipping(Z, X, clipped);
        clipping(Z, Y, Z);
        
        viewport_transofrmation(X, ctx.frame.width, ctx.frame.height);
        viewport_transofrmation(Y, ctx.frame.width, ctx.frame.height);
        viewport_transofrmation(Z, ctx.frame.width, ctx.frame.height);
        viewport_transofrmation(clipped, ctx.frame.width, ctx.frame.height);

        rasterize_triangle(X, Y, Z, ctx);
        rasterize_triangle(X, Z, clipped, ctx);
    }
    else if (X.gl_Position.z < -X.gl_Position.w && 
             Y.gl_Position.z < -Y.gl_Position.w && 
             Z.gl_Position.z >= -Z.gl_Position.w)
    {
        clipping(X, Z, X);
        clipping(Y, Z, Y);
        
        viewport_transofrmation(X, ctx.frame.width, ctx.frame.height);
        viewport_transofrmation(Y, ctx.frame.width, ctx.frame.height);
        viewport_transofrmation(Z, ctx.frame.width, ctx.frame.height);

        rasterize_triangle(X, Y, Z, ctx);
    }
    else if (X.gl_Position.z < -X.gl_Position.w && 
             Y.gl_Position.z >= -Y.gl_Position.w && 
             Z.gl_Position.z < -Z.gl_Position.w)
    {
        clipping(X, Y, X);
        clipping(Z, Y, Z);
        
        viewport_transofrmation(X, ctx.frame.width, ctx.frame.height);
        viewport_transofrmation(Y, ctx.frame.width, ctx.frame.height);
        viewport_transofrmation(Z, ctx.frame.width, ctx.frame.height);

        rasterize_triangle(X, Y, Z, ctx);
    }
    else if (X.gl_Position.z >= -X.gl_Position.w && 
             Y.gl_Position.z < -Y.gl_Position.w && 
             Z.gl_Position.z < -Z.gl_Position.w)
    {
        clipping(Y, X, Y);
        clipping(Z, X, Z);
        
        viewport_transofrmation(X, ctx.frame.width, ctx.frame.height);
        viewport_transofrmation(Y, ctx.frame.width, ctx.frame.height);
        viewport_transofrmation(Z, ctx.frame.width, ctx.frame.height);

        rasterize_triangle(X, Y, Z, ctx);
    }
}

//! [drawTrianglesImpl]
void drawTrianglesImpl(GPUContext &ctx,uint32_t nofVertices)
{
    std::thread threads[THREAD_COUNT];
    uint32_t thread_index = 0;

    InVertex X_in_vertex;
    OutVertex X_out_vertex;
    InVertex Y_in_vertex;
    OutVertex Y_out_vertex;
    InVertex Z_in_vertex;
    OutVertex Z_out_vertex;
    OutVertex clipped;

    for (uint32_t i = 0; i < nofVertices - 2; )
    {
        X_in_vertex.gl_VertexID = get_vertex_id(ctx, i++);
        Y_in_vertex.gl_VertexID = get_vertex_id(ctx, i++);
        Z_in_vertex.gl_VertexID = get_vertex_id(ctx, i++);
        
        get_vertex_attributes(ctx, X_in_vertex);
        get_vertex_attributes(ctx, Y_in_vertex);
        get_vertex_attributes(ctx, Z_in_vertex);
        
        ctx.prg.vertexShader(X_out_vertex, X_in_vertex, ctx.prg.uniforms);
        ctx.prg.vertexShader(Y_out_vertex, Y_in_vertex, ctx.prg.uniforms);
        ctx.prg.vertexShader(Z_out_vertex, Z_in_vertex, ctx.prg.uniforms);

        threads[thread_index++] = std::thread(run_thread_rasterization, std::ref(ctx), X_out_vertex,
                                              Y_out_vertex, Z_out_vertex);

        if (thread_index == THREAD_COUNT)
        {
            for (uint32_t i = 0; i < THREAD_COUNT; i++)
            {
                threads[i].join();
            }
            thread_index = 0;
        }
    }

    for (uint32_t i = 0; i < thread_index; i++)
    {
        threads[i].join();
    }
}
//! [drawTrianglesImpl]

/**
 * @brief This function reads color from texture.
 *
 * @param texture texture
 * @param uv uv coordinates
 *
 * @return color 4 floats
 */
glm::vec4 read_texture(Texture const&texture,glm::vec2 uv){
  if(!texture.data)return glm::vec4(0.f);
  auto uv1 = glm::fract(uv);
  auto uv2 = uv1*glm::vec2(texture.width-1,texture.height-1)+0.5f;
  auto pix = glm::uvec2(uv2);
  //auto t   = glm::fract(uv2);
  glm::vec4 color = glm::vec4(0.f,0.f,0.f,1.f);
  for(uint32_t c=0;c<texture.channels;++c)
    color[c] = texture.data[(pix.y*texture.width+pix.x)*texture.channels+c]/255.f;
  return color;
}

/**
 * @brief This function clears framebuffer.
 *
 * @param ctx GPUContext
 * @param r red channel
 * @param g green channel
 * @param b blue channel
 * @param a alpha channel
 */
void clear(GPUContext&ctx,float r,float g,float b,float a){
  auto&frame = ctx.frame;
  auto const nofPixels = frame.width * frame.height;
  for(size_t i=0;i<nofPixels;++i){
    frame.depth[i] = 10e10f;
    frame.color[i*4+0] = static_cast<uint8_t>(glm::min(r*255.f,255.f));
    frame.color[i*4+1] = static_cast<uint8_t>(glm::min(g*255.f,255.f));
    frame.color[i*4+2] = static_cast<uint8_t>(glm::min(b*255.f,255.f));
    frame.color[i*4+3] = static_cast<uint8_t>(glm::min(a*255.f,255.f));
  }
}

