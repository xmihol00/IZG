/*!
 * @file
 * @brief This file contains implementation of gpu
 *
 * @author Tomáš Milet, imilet@fit.vutbr.cz
 */

#include <student/gpu.hpp>

extern inline uint32_t get_vertex_id(const GPUContext &ctx, uint32_t vertex_number);

extern inline void get_vertex_attributes(const GPUContext &ctx, InVertex &in_vertex);

inline void clipping(OutVertex &current, OutVertex &second, OutVertex &clipped);

extern inline void viewport_transofrmation(OutVertex &out_vertex, uint32_t width, uint32_t height);

extern inline void clamp_color(OutFragment &out_fragment);

extern inline void per_fragment_operations(Frame &frame, const OutFragment &out_fragment, float depth, uint32_t x, uint32_t y);

void rasterize_triangle(const OutVertex &X, const OutVertex &Y, const OutVertex &Z, const GPUContext &ctx)
{
    //static int cnt = 0;
    //std::cerr << "Rasterizing Triangle..." << std::endl;
    /*if (cnt++ < 9)
    {
        return;
    }*/
    int x_min, x_max, y_min, y_max;
    
    //std::cerr << X.gl_Position.x << " " << Y.gl_Position.x << " " <<  Z.gl_Position.x << std::endl;
    //x_min = MAX(MIN(MIN(X.gl_Position.x, Y.gl_Position.x), Z.gl_Position.x), 0);
    //y_min = MAX(MIN(MIN(X.gl_Position.y, Y.gl_Position.y), Z.gl_Position.y), 0);

    //int k = std::max((int)std::min(std::min(X.gl_Position.x, Y.gl_Position.x), Z.gl_Position.x), 0);
    //int l = std::max((int)std::min(std::min(X.gl_Position.y, Y.gl_Position.y), Z.gl_Position.y), 0);
    x_min = std::max((int)std::min(std::min(X.gl_Position.x, Y.gl_Position.x), Z.gl_Position.x), 0);
    y_min = std::max((int)std::min(std::min(X.gl_Position.y, Y.gl_Position.y), Z.gl_Position.y), 0);

    x_max = std::min((int)std::max(std::max(X.gl_Position.x, Y.gl_Position.x), Z.gl_Position.x), (int)ctx.frame.width - 1);
    y_max = std::min((int)std::max(std::max(X.gl_Position.y, Y.gl_Position.y), Z.gl_Position.y), (int)ctx.frame.height - 1);
    //std::cerr << ":D " << x_min << " " << y_min << " " << x_max << " " << y_max << " " << k << " " << l << std::endl;
    if (x_min > x_max || y_min > y_max)
    {
        return;
    }
    //std::cerr << "going for it" << std::endl;

    float A1, A2, A3;
    float B1, B2, B3;
    float a_edge{(A1 = (X.gl_Position.y - Y.gl_Position.y)) * x_min + 
                 (B1 = (Y.gl_Position.x - X.gl_Position.x)) * y_min + 
                  X.gl_Position.x * Y.gl_Position.y - Y.gl_Position.x * X.gl_Position.y};
    float b_edge{(A2 = (Y.gl_Position.y - Z.gl_Position.y)) * x_min + 
                 (B2 = (Z.gl_Position.x - Y.gl_Position.x)) * y_min + 
                  Y.gl_Position.x * Z.gl_Position.y - Z.gl_Position.x * Y.gl_Position.y};
    float c_edge{(A3 = (Z.gl_Position.y - X.gl_Position.y)) * x_min + 
                 (B3 = (X.gl_Position.x - Z.gl_Position.x)) * y_min + 
                  Z.gl_Position.x * X.gl_Position.y - X.gl_Position.x * Z.gl_Position.y};

    float a_dot{B1 * B1 + A1 * A1};
    float b_dot{B1 * B3 + A1 * A3};
    float c_dot{B3 * B3 + A3 * A3};
    float d_dot;
    float e_dot;
    float Ap, Bp;
    float l0, l1, l2;
    float l03D, l13D, l23D;
    float l0h0, l1h1, l2h2;
    float s;
    float A1_positive = -A1;
    float A3_positive = A3;

    float inverz_denometer{1.0f / (a_dot * c_dot - b_dot * b_dot)};

    InFragment in_fragment;
    OutFragment out_fragment;
    int iterator{1};
    for (int y = y_min; y <= y_max; y++)
    {
        in_fragment.gl_FragCoord.y = (float)y + 0.5;
        Bp = in_fragment.gl_FragCoord.y - X.gl_Position.y;
        for (int x = x_min; x != x_max; x += iterator)
        {   
            //std::cerr << x << " " << x_min << " " << x_max << std::endl;
            if (a_edge >= 0 && b_edge >= 0 && c_edge >= 0)
            {   
                in_fragment.gl_FragCoord.x = (float)x + 0.5;

                Ap = in_fragment.gl_FragCoord.x - X.gl_Position.x;
                d_dot = Ap * B1 + Bp * A1_positive;
                e_dot = -Ap * B3 + Bp * A3_positive;
                l1 = (c_dot * d_dot + b_dot * e_dot) * inverz_denometer;
                l2 = (a_dot * e_dot + b_dot * d_dot) * inverz_denometer;
                l0 = 1.0f - l1 - l2;
                in_fragment.gl_FragCoord.z = X.gl_Position.z * l0 + Y.gl_Position.z * l1 + Z.gl_Position.z * l2;

                l0h0 = l0 / X.gl_Position.w;
                l1h1 = l1 / Y.gl_Position.w;
                l2h2 = l2 / Z.gl_Position.w;
                s = l0h0 + l1h1 + l2h2;
                l03D = l0h0 / s;
                l13D = l1h1 / s;
                l23D = l2h2 / s;
                
                /*if (in_fragment.gl_FragCoord.y == 0.5f && in_fragment.gl_FragCoord.x < 10.0)
                {
                    std::cerr << "coordinates my      X: [" << X.gl_Position.x << ", " << X.gl_Position.y << "], Y: [" << Y.gl_Position.x << ", " << Y.gl_Position.y << "], Z: [" << Z.gl_Position.x << ", " << Z.gl_Position.y << "], P: [" << in_fragment.gl_FragCoord.x << ", " << in_fragment.gl_FragCoord.y << "]" << std::endl;
                    std::cerr << "barycentrics my    l0: " << l0 << ", l1: " << l1 << ", l2: " << l2 << std::endl;
                    std::cerr << "barycentrics 3D my l0: " << l03D << ", l1: " << l13D << ", l2: " << l23D << std::endl;
                    std::cerr << "s   : " << s << std::endl;
                    std::cerr << "clip: " << X.gl_Position.w << " " << Y.gl_Position.w << " " << Z.gl_Position.w << std::endl;
                }*/

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
                //std::cerr << "  x: " << x << "  y: " << y << std::endl;
                per_fragment_operations(ctx.frame, out_fragment, in_fragment.gl_FragCoord.z, x, y);
            }

            a_edge += A1;
            b_edge += A2;
            c_edge += A3;
        }

        std::swap(x_min, x_max);
        iterator = -iterator;
        A1 = -A1;
        A2 = -A2;
        A3 = -A3;
        a_edge += B1;
        b_edge += B2;
        c_edge += B3;
    }
}

//! [drawTrianglesImpl]
void drawTrianglesImpl(GPUContext &ctx,uint32_t nofVertices)
{
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

        //std::cerr << X_out_vertex.gl_Position.x << " " << Y_out_vertex.gl_Position.x << " " <<  Z_out_vertex.gl_Position.x << std::endl;
        if (X_out_vertex.gl_Position.z >= -X_out_vertex.gl_Position.w && 
            Y_out_vertex.gl_Position.z >= -Y_out_vertex.gl_Position.w && 
            Z_out_vertex.gl_Position.z >= -Z_out_vertex.gl_Position.w)
        {
            viewport_transofrmation(X_out_vertex, ctx.frame.width, ctx.frame.height);
            viewport_transofrmation(Y_out_vertex, ctx.frame.width, ctx.frame.height);
            viewport_transofrmation(Z_out_vertex, ctx.frame.width, ctx.frame.height);

            rasterize_triangle(X_out_vertex, Y_out_vertex, Z_out_vertex, ctx);
        }
        else if (X_out_vertex.gl_Position.z < -X_out_vertex.gl_Position.w && 
                 Y_out_vertex.gl_Position.z >= -Y_out_vertex.gl_Position.w && 
                 Z_out_vertex.gl_Position.z >= -Z_out_vertex.gl_Position.w)
        {
            clipped = X_out_vertex;
            clipping(X_out_vertex, Z_out_vertex, clipped);
            clipping(X_out_vertex, Y_out_vertex, X_out_vertex);
            
            viewport_transofrmation(X_out_vertex, ctx.frame.width, ctx.frame.height);
            viewport_transofrmation(Y_out_vertex, ctx.frame.width, ctx.frame.height);
            viewport_transofrmation(Z_out_vertex, ctx.frame.width, ctx.frame.height);
            viewport_transofrmation(clipped, ctx.frame.width, ctx.frame.height);

            rasterize_triangle(X_out_vertex, Y_out_vertex, Z_out_vertex, ctx);
            rasterize_triangle(clipped, Y_out_vertex, X_out_vertex, ctx);
        }
        else if (X_out_vertex.gl_Position.z >= -X_out_vertex.gl_Position.w && 
                 Y_out_vertex.gl_Position.z < -Y_out_vertex.gl_Position.w && 
                 Z_out_vertex.gl_Position.z >= -Z_out_vertex.gl_Position.w)
        {
            clipped = Y_out_vertex;
            clipping(Y_out_vertex, X_out_vertex, clipped);
            clipping(Y_out_vertex, Z_out_vertex, Y_out_vertex);
            
            viewport_transofrmation(X_out_vertex, ctx.frame.width, ctx.frame.height);
            viewport_transofrmation(Y_out_vertex, ctx.frame.width, ctx.frame.height);
            viewport_transofrmation(Z_out_vertex, ctx.frame.width, ctx.frame.height);
            viewport_transofrmation(clipped, ctx.frame.width, ctx.frame.height);

            rasterize_triangle(X_out_vertex, Y_out_vertex, Z_out_vertex, ctx);
            rasterize_triangle(Z_out_vertex, Y_out_vertex, clipped, ctx);
        }
        else if (X_out_vertex.gl_Position.z >= -X_out_vertex.gl_Position.w && 
                 Y_out_vertex.gl_Position.z >= -Y_out_vertex.gl_Position.w && 
                 Z_out_vertex.gl_Position.z < -Z_out_vertex.gl_Position.w)
        {
            clipped = Z_out_vertex;
            clipping(Z_out_vertex, X_out_vertex, clipped);
            clipping(Z_out_vertex, Y_out_vertex, Z_out_vertex);
            
            viewport_transofrmation(X_out_vertex, ctx.frame.width, ctx.frame.height);
            viewport_transofrmation(Y_out_vertex, ctx.frame.width, ctx.frame.height);
            viewport_transofrmation(Z_out_vertex, ctx.frame.width, ctx.frame.height);
            viewport_transofrmation(clipped, ctx.frame.width, ctx.frame.height);

            rasterize_triangle(X_out_vertex, Y_out_vertex, Z_out_vertex, ctx);
            rasterize_triangle(X_out_vertex, Z_out_vertex, clipped, ctx);
        }
        else if (X_out_vertex.gl_Position.z < -X_out_vertex.gl_Position.w && 
                 Y_out_vertex.gl_Position.z < -Y_out_vertex.gl_Position.w && 
                 Z_out_vertex.gl_Position.z >= -Z_out_vertex.gl_Position.w)
        {
            clipping(X_out_vertex, Z_out_vertex, X_out_vertex);
            clipping(Y_out_vertex, Z_out_vertex, Y_out_vertex);
            
            viewport_transofrmation(X_out_vertex, ctx.frame.width, ctx.frame.height);
            viewport_transofrmation(Y_out_vertex, ctx.frame.width, ctx.frame.height);
            viewport_transofrmation(Z_out_vertex, ctx.frame.width, ctx.frame.height);

            rasterize_triangle(X_out_vertex, Y_out_vertex, Z_out_vertex, ctx);
        }
        else if (X_out_vertex.gl_Position.z < -X_out_vertex.gl_Position.w && 
                 Y_out_vertex.gl_Position.z >= -Y_out_vertex.gl_Position.w && 
                 Z_out_vertex.gl_Position.z < -Z_out_vertex.gl_Position.w)
        {
            clipping(X_out_vertex, Y_out_vertex, X_out_vertex);
            clipping(Z_out_vertex, Y_out_vertex, Z_out_vertex);
            
            viewport_transofrmation(X_out_vertex, ctx.frame.width, ctx.frame.height);
            viewport_transofrmation(Y_out_vertex, ctx.frame.width, ctx.frame.height);
            viewport_transofrmation(Z_out_vertex, ctx.frame.width, ctx.frame.height);

            rasterize_triangle(X_out_vertex, Y_out_vertex, Z_out_vertex, ctx);
        }
        else if (X_out_vertex.gl_Position.z >= -X_out_vertex.gl_Position.w && 
                 Y_out_vertex.gl_Position.z < -Y_out_vertex.gl_Position.w && 
                 Z_out_vertex.gl_Position.z < -Z_out_vertex.gl_Position.w)
        {
            clipping(Y_out_vertex, X_out_vertex, Y_out_vertex);
            clipping(Z_out_vertex, X_out_vertex, Z_out_vertex);
            
            viewport_transofrmation(X_out_vertex, ctx.frame.width, ctx.frame.height);
            viewport_transofrmation(Y_out_vertex, ctx.frame.width, ctx.frame.height);
            viewport_transofrmation(Z_out_vertex, ctx.frame.width, ctx.frame.height);

            rasterize_triangle(X_out_vertex, Y_out_vertex, Z_out_vertex, ctx);
        }
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

