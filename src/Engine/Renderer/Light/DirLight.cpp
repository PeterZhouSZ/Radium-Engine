#include <Engine/Renderer/Light/DirLight.hpp>

#include <Engine/Renderer/RenderTechnique/RenderParameters.hpp>

namespace Ra
{

    Engine::DirectionalLight::DirectionalLight()
        : Light( Light::DIRECTIONAL )
        , m_direction( 0, -1, 0 )
    {
    }

    Engine::DirectionalLight::~DirectionalLight()
    {
    }

    void Engine::DirectionalLight::getRenderParameters( RenderParameters& params )
    {
        Light::getRenderParameters( params );

        params.addParameter( "light.directional.direction", m_direction );
    }

}
