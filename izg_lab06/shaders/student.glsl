#ifdef VERTEX_SHADER
//#define MAKE_STUDENT_RELEASE
uniform mat4 view;
uniform mat4 proj;
#line 5

vec4 positions[] = vec4[](
                            vec4(-2.0f,  0.0f, -2.0f, 1.0f),
                            vec4( 2.0f,  0.0f, -2.0f, 1.0f),
                            vec4(-2.0f,  0.0f,  2.0f, 1.0f),
                            vec4(-2.0f,  0.0f,  2.0f, 1.0f),
                            vec4( 2.0f,  0.0f, -2.0f, 1.0f),
                            vec4( 2.0f,  0.0f,  2.0f, 1.0f),
                            vec4(-0.5f,  0.0f, -0.5f, 1.0f),
                            vec4( 0.5f,  0.0f, -0.5f, 1.0f),
                            vec4(-0.5f,  1.0f, -0.5f, 1.0f),
                            vec4(-0.5f,  1.0f, -0.5f, 1.0f),
                            vec4( 0.5f,  0.0f, -0.5f, 1.0f),
                            vec4( 0.5f,  1.0f, -0.5f, 1.0f),
                            vec4(-0.5f,  0.0f, -0.5f, 1.0f),
                            vec4(-0.5f,  0.0f,  0.5f, 1.0f),
                            vec4(-0.5f,  1.0f, -0.5f, 1.0f),
                            vec4(-0.5f,  1.0f, -0.5f, 1.0f),
                            vec4(-0.5f,  0.0f,  0.5f, 1.0f),
                            vec4(-0.5f,  1.0f,  0.5f, 1.0f),
                            vec4( 0.0f,  2.0f,  0.0f, 1.0f),
                            vec4( 0.5f,  1.0f,  0.5f, 1.0f),
                            vec4( 0.5f,  1.0f, -0.5f, 1.0f),
                            vec4( 0.0f,  2.0f,  0.0f, 1.0f),
                            vec4( 0.5f,  1.0f,  0.5f, 1.0f),
                            vec4(-0.5f,  1.0f,  0.5f, 1.0f),
                            vec4( 0.0f,  2.0f,  0.0f, 1.0f),
                            vec4(-0.5f,  1.0f, -0.5f, 1.0f),
                            vec4(-0.5f,  1.0f,  0.5f, 1.0f),
                            vec4( 0.0f,  2.0f,  0.0f, 1.0f),
                            vec4(-0.5f,  1.0f, -0.5f, 1.0f),
                            vec4( 0.5f,  1.0f, -0.5f, 1.0f),
                            vec4( 0.5f,  0.0f,  0.5f, 1.0f),
                            vec4(-0.5f,  0.0f,  0.5f, 1.0f),
                            vec4( 0.5f,  1.0f,  0.5f, 1.0f),
                            vec4( 0.5f,  1.0f,  0.5f, 1.0f),
                            vec4(-0.5f,  0.0f,  0.5f, 1.0f),
                            vec4(-0.5f,  1.0f,  0.5f, 1.0f),
                            vec4( 0.5f,  0.0f,  0.5f, 1.0f),
                            vec4( 0.5f,  0.0f, -0.5f, 1.0f),
                            vec4( 0.5f,  1.0f,  0.5f, 1.0f),
                            vec4( 0.5f,  1.0f,  0.5f, 1.0f),
                            vec4( 0.5f,  0.0f, -0.5f, 1.0f),
                            vec4( 0.5f,  1.0f, -0.5f, 1.0f)
                        );

vec3 coordinates[] = vec3[](
                                vec3(0.0f, 0.0f, 0.0f),
                                vec3(2.0f, 0.0f, 0.0f),
                                vec3(0.0f, 2.0f, 0.0f),
                                vec3(0.0f, 2.0f, 0.0f),
                                vec3(2.0f, 0.0f, 0.0f),
                                vec3(2.0f, 2.0f, 0.0f),
                                vec3(0.75f, 0.75f, 0.0f),
                                vec3(1.25f, 0.75f, 0.0f),
                                vec3(0.75f, 0.75f, 1.0f),
                                vec3(0.75f, 0.75f, 1.0f),
                                vec3(1.25f, 0.75f, 0.0f),
                                vec3(1.25f, 0.75f, 1.0f),
                                vec3(0.75f, 0.75f, 0.0f),
                                vec3(0.75f, 1.25f, 0.0f),
                                vec3(0.75f, 0.75f, 1.0f),
                                vec3(0.75f, 0.75f, 1.0f),
                                vec3(0.75f, 1.25f, 0.0f),
                                vec3(0.75f, 1.25f, 1.0f),
                                vec3(0.00f, 0.00f, 2.0f),
                                vec3(1.25f, 1.25f, 1.0f),
                                vec3(1.25f, 0.75f, 1.0f),
                                vec3(0.00f, 0.00f, 2.0f),
                                vec3(1.25f, 1.25f, 1.0f),
                                vec3(0.75f, 1.25f, 1.0f),
                                vec3(0.00f, 0.00f, 2.0f),
                                vec3(0.75f, 0.75f, 1.0f),
                                vec3(0.75f, 1.25f, 1.0f),
                                vec3(0.00f, 0.00f, 2.0f),
                                vec3(0.75f, 0.75f, 1.0f),
                                vec3(1.25f, 0.75f, 1.0f),
                                vec3(1.25f, 1.25f, 0.0f),
                                vec3(1.25f, 0.75f, 0.0f),
                                vec3(1.25f, 1.25f, 1.0f),
                                vec3(1.25f, 1.25f, 1.0f),
                                vec3(1.25f, 0.75f, 0.0f),
                                vec3(1.25f, 0.75f, 1.0f),
                                vec3(1.25f, 1.25f, 0.0f),
                                vec3(1.25f, 0.75f, 0.0f),
                                vec3(1.25f, 1.25f, 1.0f),
                                vec3(1.25f, 1.25f, 1.0f),
                                vec3(1.25f, 0.75f, 0.0f),
                                vec3(1.25f, 0.75f, 1.0f)
                           );

mat4 mvp = proj*view;
out highp vec3 vCoord;
void main()
{
    if (gl_VertexID >= 0 && gl_VertexID < 42)
    {
        gl_Position = mvp * positions[gl_VertexID];
        vCoord = coordinates[gl_VertexID];
        return;
    }
    else
    {
        gl_Position = vec4(0.0f, 0.0f, 0.0f, 0.0f);
        vCoord = vec3(-1.0f, -1.0f, -1.0f);
    }
}
#endif



#ifdef FRAGMENT_SHADER

out highp vec4 fColor;
in highp vec3 vCoord;

void main()
{
    if (vCoord.z > 0.0f && vCoord.z <= 1.0f)
    {
        fColor = vec4(0.0f, 0.0f, 0.0f, 1.0f);
    }

    if (vCoord.z > 0.1f && vCoord.z < 0.9f)
    {
        if (vCoord.x < 1.20f && vCoord.x > 0.80f)
        {
            fColor = vec4(0.5f, 0.5f, 0.0f, 0.5f);
        }
        if (vCoord.y < 1.20f && vCoord.y > 0.80f)
        {
            fColor = vec4(0.5f, 0.5f, 0.0f, 0.5f);
        }
        return;
    }

    if (vCoord.z > 1.0f && vCoord.z <= 2.0f)
    {
        fColor = (2.0f - vCoord.z) * vec4(0.0f, 0.0f, 0.0f, 1.0f) + (vCoord.z - 1) * vec4(1.0f, 0.0f, 0.0f, 1.0f);
        return;
    }

    for (highp float i = 0.25f; i < 1.8f; i += 0.5f)
    {
        for (highp float j = 0.25f; j < 1.8f; j += 0.5f)
        {
            if (vCoord.z == 0.0f && distance(vCoord, vec3(i, j, 0.0f)) < 0.25f)
            {
                fColor = vec4(0.0f, 1.0f, 0.0f, 1.0f);
                return;        
            }
        }
    }

    if (vCoord.z == 0.0f)
    {
        fColor = vec4(0.0f, 0.5f, 0.0f, 1.0f);
    }
}
#endif
