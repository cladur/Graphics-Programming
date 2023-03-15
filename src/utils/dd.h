#include "debug_draw.hpp"

#include "glm/glm.hpp"
#include <glad/glad.h>

extern float viewportWidth;
extern float viewportHeight;

class DDRenderInterfaceCoreGL final
    : public dd::RenderInterface {
public:
    void drawPointList(const dd::DrawVertex* points, int count, bool depthEnabled) override;
    void drawLineList(const dd::DrawVertex* lines, int count, bool depthEnabled) override;
    void drawGlyphList(const dd::DrawVertex* glyphs, int count, dd::GlyphTextureHandle glyphTex) override;
    dd::GlyphTextureHandle createGlyphTexture(int width, int height, const void* pixels) override;
    void destroyGlyphTexture(dd::GlyphTextureHandle glyphTex) override;

    DDRenderInterfaceCoreGL();
    ~DDRenderInterfaceCoreGL();
    void setupShaderPrograms();
    void setupVertexBuffers();
    static GLuint handleToGL(dd::GlyphTextureHandle handle);
    static dd::GlyphTextureHandle GLToHandle(const GLuint id);
    static void checkGLError(const char* file, const int line);
    static void compileShader(const GLuint shader);
    static void linkProgram(const GLuint program);

    glm::mat4 mvpMatrix;

private:
    GLuint linePointProgram;
    GLint linePointProgram_MvpMatrixLocation;

    GLuint textProgram;
    GLint textProgram_GlyphTextureLocation;
    GLint textProgram_ScreenDimensions;

    GLuint linePointVAO;
    GLuint linePointVBO;

    GLuint textVAO;
    GLuint textVBO;

    static const char* linePointVertShaderSrc;
    static const char* linePointFragShaderSrc;

    static const char* textVertShaderSrc;
    static const char* textFragShaderSrc;

}; // class DDRenderInterfaceCoreGL
