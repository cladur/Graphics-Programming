void renderCube();

class Skybox {
public:
    Skybox(const char* hdri_path);
    ~Skybox();

    void generateCubemap(unsigned int cubemapSize);
    void generateIrradianceMap();
    void render(unsigned int cubemap_size);

    unsigned int getIrradianceMap() const { return irradianceMap; }
    unsigned int getPrefilterMap() const { return prefilterMap; }
    unsigned int getBrdfLUTTexture() const { return brdfLUTTexture; }
    unsigned int getEnvCubemap() const { return envCubemap; }

private:
    unsigned int hdrTexture;
    unsigned int captureFBO;
    unsigned int captureRBO;
    unsigned int envCubemap;
    unsigned int irradianceMap;
    unsigned int prefilterMap;
    unsigned int brdfLUTTexture;
};
