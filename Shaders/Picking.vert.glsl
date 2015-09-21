layout (location = 0) in vec3 inPosition;


struct Transform
{
    mat4 model;
    mat4 view;
    mat4 proj;
    mat4 mvp;
    mat4 modelView;
    mat4 worldNormal;
    mat4 viewNormal;
};


uniform int drawFixedSize;
uniform Transform transform;

void main()
{
    mat4 mvp = transform.mvp;
    if ( drawFixedSize > 0 )
    {
        // distance to camera
        float d = length(transform.modelView[3].xyz);
        mat3 scale3 = mat3(d);
        mat4 scale = mat4(scale3);
        mat4 model = transform.model * scale;
        mvp = transform.proj * transform.view* model;
    }
    gl_Position = mvp * vec4(inPosition, 1.0);
}