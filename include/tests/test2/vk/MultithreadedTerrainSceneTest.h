#pragma once

#include <framework/VKTest.h>

#include <base/vkx/ShaderModule.h>
#include <tests/test2/BaseTerrainSceneTest.h>

namespace tests {
namespace test_vk {
class MultithreadedTerrainSceneTest : public BaseTerrainSceneTest, public framework::VKTest
{
  public:
    MultithreadedTerrainSceneTest(bool benchmarkMode, float benchmarkTime, int n, int nt);

    void setup() override;
    void run() override;
    void teardown() override;

  private:
    struct PerThreadSecondaryCommandPool
    {
        vk::CommandPool cmdPool;
        std::vector<vk::CommandBuffer> cmdBuffers;
    };

    void createVbo();
    void createIbo();
    void createCommandBuffers();
    void createSecondaryCommandBuffers();
    void createSemaphores();
    void createFences();
    void createRenderPass();
    void createFramebuffers();
    void createShaders();
    void createPipelineLayout();
    void createPipeline();

    void destroyPipeline();
    void destroyPipelineLayout();
    void destroyShaders();
    void destroyFramebuffers();
    void destroyRenderPass();
    void destroyFences();
    void destroySemaphores();
    void destroySecondaryCommandBuffers();
    void destroyCommandBuffers();
    void destroyIbo();
    void destroyVbo();

    std::vector<vk::PipelineShaderStageCreateInfo> getShaderStages() const;
    uint32_t getNextFrameIndex() const;

    void prepareSecondaryCommandBuffer(std::size_t frameIndex, std::size_t bufferIndex) const;
    void prepareCommandBuffer(std::size_t frameIndex) const;
    void submitCommandBuffer(std::size_t frameIndex) const;
    void presentFrame(std::size_t frameIndex) const;
    void createTable();

    base::vkx::Buffer _vbo;
    base::vkx::Buffer _ibo;
    vk::CommandPool _cmdPool;
    std::vector<vk::CommandBuffer> _cmdBuffers;
    std::vector<PerThreadSecondaryCommandPool> _threadCmdPools;
    std::vector<vk::Fence> _fences;
    mutable std::size_t _semaphoreIndex;
    std::vector<vk::Semaphore> _acquireSemaphores;
    std::vector<vk::Semaphore> _renderSemaphores;
    vk::RenderPass _renderPass;
    std::vector<vk::Framebuffer> _framebuffers;
    vk::DescriptorSetLayout _setLayout;
    vk::PipelineLayout _pipelineLayout;
    vk::Pipeline _pipeline;
    base::vkx::ShaderModule _vertexModule;
    base::vkx::ShaderModule _fragmentModule;
};
}
}
