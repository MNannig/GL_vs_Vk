#include <tests/test1/vk/MultithreadedBallsSceneTest.h>

#include <base/ScopedTimer.h>

#include <glm/vec4.hpp>
#include <vulkan/vulkan.hpp>

#include <array>
#include <stdexcept>
#include <thread>
#include <vector>

namespace tests {
namespace test_vk {
MultithreadedBallsSceneTest::MultithreadedBallsSceneTest(bool benchmarkMode, int benchmark_stat, float benchmarkTime, int n, int nt)
    : BaseBallsSceneTest()
    , VKTest("MultithreadedBallsSceneTest", benchmarkMode, benchmark_stat, benchmarkTime, n, nt)
    , _semaphoreIndex(0u)
{
}

void MultithreadedBallsSceneTest::setup()
{
    VKTest::setup();
    initTestState(_n);

    createCommandBuffers();
    createSecondaryCommandBuffers();
    createVbo();
    createSemaphores();
    createFences();
    createRenderPass();
    createFramebuffers();
    createShaders();
    createPipelineLayout();
    createPipeline();
}

void MultithreadedBallsSceneTest::run()
{
    //printf("starting vk test1, n=%i  nt=%i gl=%i\n", _n, _nt, _benchmark_stat);
    while (!window().shouldClose()) {
        TIME_RESET("Frame times:");

        auto frameIndex = getNextFrameIndex();

        prepareCommandBuffer(frameIndex);
        submitCommandBuffer(frameIndex);
        presentFrame(frameIndex);

        window().update();

        if (processFrameTime(window().frameTime())) {
            break; // Benchmarking is complete
        }
    }
}

void MultithreadedBallsSceneTest::teardown()
{
    device().waitIdle();

    destroyPipeline();
    destroyPipelineLayout();
    destroyShaders();
    destroyFramebuffers();
    destroyRenderPass();
    destroyFences();
    destroySemaphores();
    destroyVbo();
    destroySecondaryCommandBuffers();
    destroyCommandBuffers();

    destroyTestState();
    VKTest::teardown();
}

void MultithreadedBallsSceneTest::createCommandBuffers()
{
    vk::CommandPoolCreateFlags cmdPoolFlags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
    _cmdPool = device().createCommandPool({cmdPoolFlags, queues().familyIndex()});
    _cmdBuffers = device().allocateCommandBuffers(
        {_cmdPool, vk::CommandBufferLevel::ePrimary, static_cast<uint32_t>(window().swapchainImages().size())});
}

void MultithreadedBallsSceneTest::createSecondaryCommandBuffers()
{

    //static const std::size_t threads = std::thread::hardware_concurrency();

    static std::size_t threads = -1;
    if(_nt == 0){
        threads = std::thread::hardware_concurrency();
    }
    else{
        threads = _nt;
    }

    _threadCmdPools.resize(threads);
    for (auto& sndCmdPool : _threadCmdPools) {
        vk::CommandPoolCreateFlags cmdPoolFlags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
        sndCmdPool.cmdPool = device().createCommandPool({cmdPoolFlags, queues().familyIndex()});
        sndCmdPool.cmdBuffers =
            device().allocateCommandBuffers({sndCmdPool.cmdPool, vk::CommandBufferLevel::eSecondary,
                                             static_cast<uint32_t>(window().swapchainImages().size())});
    }
}

void MultithreadedBallsSceneTest::createVbo()
{
    vk::DeviceSize size = vertices().size() * sizeof(vertices().front());
    vk::BufferUsageFlags usage = vk::BufferUsageFlagBits::eVertexBuffer;

    base::vkx::Buffer stagingBuffer = memory().createStagingBuffer(size);
    {
        auto vboMemory = device().mapMemory(stagingBuffer.memory, stagingBuffer.offset, stagingBuffer.size, {});
        std::memcpy(vboMemory, vertices().data(), static_cast<std::size_t>(stagingBuffer.size));
        device().unmapMemory(stagingBuffer.memory);
    }
    _vbo = memory().copyToDeviceLocalMemory(stagingBuffer, usage, _cmdBuffers.front(), queues().queue());

    memory().destroyBuffer(stagingBuffer);
}

void MultithreadedBallsSceneTest::createSemaphores()
{
    for (std::size_t i = 0; i < window().swapchainImages().size(); ++i) {
        _acquireSemaphores.push_back(device().createSemaphore({}));
        _renderSemaphores.push_back(device().createSemaphore({}));
    }
}

void MultithreadedBallsSceneTest::createFences()
{
    _fences.resize(_cmdBuffers.size());
    for (vk::Fence& fence : _fences) {
        fence = device().createFence({vk::FenceCreateFlagBits::eSignaled});
    }
}

void MultithreadedBallsSceneTest::createRenderPass()
{
    vk::AttachmentDescription attachment{{},
                                         window().swapchainImageFormat(),
                                         vk::SampleCountFlagBits::e1,
                                         vk::AttachmentLoadOp::eClear,
                                         vk::AttachmentStoreOp::eStore,
                                         vk::AttachmentLoadOp::eDontCare,
                                         vk::AttachmentStoreOp::eDontCare,
                                         vk::ImageLayout::eUndefined,
                                         vk::ImageLayout::ePresentSrcKHR};
    vk::AttachmentReference colorAttachment{0, vk::ImageLayout::eColorAttachmentOptimal};
    vk::SubpassDescription subpassDesc{
        {}, vk::PipelineBindPoint::eGraphics, 0, nullptr, 1, &colorAttachment, nullptr, nullptr, 0, nullptr};

    vk::RenderPassCreateInfo renderPassInfo{{}, 1, &attachment, 1, &subpassDesc, 0, nullptr};
    _renderPass = device().createRenderPass(renderPassInfo);
}

void MultithreadedBallsSceneTest::createFramebuffers()
{
    _framebuffers.reserve(window().swapchainImages().size());
    for (const vk::ImageView& imageView : window().swapchainImageViews()) {
        vk::FramebufferCreateInfo framebufferInfo{{}, _renderPass, 1, &imageView, window().size().x, window().size().y,
                                                  1};
        _framebuffers.push_back(device().createFramebuffer(framebufferInfo));
    }
}

void MultithreadedBallsSceneTest::createShaders()
{
    _vertexModule = base::vkx::ShaderModule{device(), "resources/test1/shaders/vk_shader.vert.spv"};
    _fragmentModule = base::vkx::ShaderModule{device(), "resources/test1/shaders/vk_shader.frag.spv"};
}

void MultithreadedBallsSceneTest::createPipelineLayout()
{
    std::vector<vk::DescriptorSetLayoutBinding> bindings;
    vk::DescriptorSetLayoutCreateInfo setLayoutInfo{{}, static_cast<uint32_t>(bindings.size()), bindings.data()};
    _setLayout = device().createDescriptorSetLayout(setLayoutInfo);

    vk::PushConstantRange pushConstantRanges[] = {
        {vk::ShaderStageFlagBits::eVertex, 0, sizeof(glm::vec4)}, // position
        {vk::ShaderStageFlagBits::eFragment, sizeof(glm::vec4), sizeof(glm::vec4)} // color
    };

    vk::PipelineLayoutCreateInfo pipelineLayoutInfo{{}, 1, &_setLayout, 2, &pushConstantRanges[0]};
    _pipelineLayout = device().createPipelineLayout(pipelineLayoutInfo);
}

void MultithreadedBallsSceneTest::createPipeline()
{
    // Shader stages
    std::vector<vk::PipelineShaderStageCreateInfo> shaderStages = getShaderStages();

    // Vertex input state
    std::vector<vk::VertexInputBindingDescription> vertexBindingDescriptions{
        {0, sizeof(glm::vec4), vk::VertexInputRate::eVertex} // Binding #0 - vertex input data
    };
    std::vector<vk::VertexInputAttributeDescription> vertexAttributeDescription{
        {0, 0, vk::Format::eR32G32B32A32Sfloat, 0} // Attribute #0 (from binding #0) - vec4
    };
    vk::PipelineVertexInputStateCreateInfo vertexInputState{{},
                                                            static_cast<uint32_t>(vertexBindingDescriptions.size()),
                                                            vertexBindingDescriptions.data(),
                                                            static_cast<uint32_t>(vertexAttributeDescription.size()),
                                                            vertexAttributeDescription.data()};

    // Input assembly state
    vk::PipelineInputAssemblyStateCreateInfo inputAssemblyState{{}, vk::PrimitiveTopology::eTriangleList, VK_FALSE};

    // Viewport state
    vk::Viewport viewport{0.0f, 0.0f, static_cast<float>(window().size().x), static_cast<float>(window().size().y),
                          0.0f, 1.0f};
    vk::Rect2D scissor{{0, 0}, {static_cast<uint32_t>(window().size().x), static_cast<uint32_t>(window().size().y)}};
    vk::PipelineViewportStateCreateInfo viewportState{{}, 1, &viewport, 1, &scissor};

    // Rasterization state
    vk::PipelineRasterizationStateCreateInfo rasterizationState{{},
                                                                VK_FALSE,
                                                                VK_FALSE,
                                                                vk::PolygonMode::eFill,
                                                                vk::CullModeFlagBits::eNone,
                                                                vk::FrontFace::eCounterClockwise,
                                                                VK_FALSE,
                                                                0.0f,
                                                                0.0f,
                                                                0.0f,
                                                                1.0f};

    // Multisample state
    vk::PipelineMultisampleStateCreateInfo multisampleState{
        {}, vk::SampleCountFlagBits::e1, VK_FALSE, 0.0f, nullptr, VK_FALSE, VK_FALSE};

    // ColorBlend state
    vk::PipelineColorBlendAttachmentState colorBlendAttachmentState{VK_FALSE};
    colorBlendAttachmentState.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
                                               vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
    vk::PipelineColorBlendStateCreateInfo colorBlendState{
        {}, VK_FALSE, vk::LogicOp::eClear, 1, &colorBlendAttachmentState};

    // Pipeline creation
    vk::GraphicsPipelineCreateInfo pipelineInfo{{},
                                                static_cast<uint32_t>(shaderStages.size()),
                                                shaderStages.data(),
                                                &vertexInputState,
                                                &inputAssemblyState,
                                                nullptr,
                                                &viewportState,
                                                &rasterizationState,
                                                &multisampleState,
                                                nullptr,
                                                &colorBlendState,
                                                nullptr,
                                                _pipelineLayout,
                                                _renderPass,
                                                0};
    _pipeline = device().createGraphicsPipeline({}, pipelineInfo);
}

void MultithreadedBallsSceneTest::destroyPipeline()
{
    device().destroyPipeline(_pipeline);
    device().destroyDescriptorSetLayout(_setLayout);
}

void MultithreadedBallsSceneTest::destroyPipelineLayout()
{
    device().destroyPipelineLayout(_pipelineLayout);
}

void MultithreadedBallsSceneTest::destroyShaders()
{
    _vertexModule = {};
    _fragmentModule = {};
}

void MultithreadedBallsSceneTest::destroyFramebuffers()
{
    for (const vk::Framebuffer& framebuffer : _framebuffers) {
        device().destroyFramebuffer(framebuffer);
    }
}

void MultithreadedBallsSceneTest::destroyRenderPass()
{
    device().destroyRenderPass(_renderPass);
}

void MultithreadedBallsSceneTest::destroyFences()
{
    for (const vk::Fence& fence : _fences) {
        device().destroyFence(fence);
    }
    _fences.clear();
}

void MultithreadedBallsSceneTest::destroySemaphores()
{
    for (const auto& acquireSemaphore : _acquireSemaphores) {
        device().destroySemaphore(acquireSemaphore);
    }
    for (const auto& renderSemaphore : _renderSemaphores) {
        device().destroySemaphore(renderSemaphore);
    }
    _acquireSemaphores.clear();
    _renderSemaphores.clear();
}

void MultithreadedBallsSceneTest::destroyVbo()
{
    memory().destroyBuffer(_vbo);
}

void MultithreadedBallsSceneTest::destroySecondaryCommandBuffers()
{
    for (auto& threadCmdPool : _threadCmdPools) {
        device().destroyCommandPool(threadCmdPool.cmdPool);
        threadCmdPool.cmdBuffers.clear();
    }
    _threadCmdPools.clear();
}

void MultithreadedBallsSceneTest::destroyCommandBuffers()
{
    device().destroyCommandPool(_cmdPool);
    _cmdBuffers.clear();
}

std::vector<vk::PipelineShaderStageCreateInfo> MultithreadedBallsSceneTest::getShaderStages() const
{
    std::vector<vk::PipelineShaderStageCreateInfo> stages;

    vk::PipelineShaderStageCreateInfo vertexStage{{}, vk::ShaderStageFlagBits::eVertex, _vertexModule, "main", nullptr};
    stages.push_back(vertexStage);

    vk::PipelineShaderStageCreateInfo fragmentStage{
        {}, vk::ShaderStageFlagBits::eFragment, _fragmentModule, "main", nullptr};
    stages.push_back(fragmentStage);

    return stages;
}

uint32_t MultithreadedBallsSceneTest::getNextFrameIndex() const
{
    _semaphoreIndex = (_semaphoreIndex + 1) % window().swapchainImages().size();

    auto nextFrameAcquireStatus =
        device().acquireNextImageKHR(window().swapchain(), UINT64_MAX, _acquireSemaphores[_semaphoreIndex], {});

    if (nextFrameAcquireStatus.result != vk::Result::eSuccess) {
        throw std::system_error(nextFrameAcquireStatus.result, "Error during acquiring next frame index");
    }

    return nextFrameAcquireStatus.value;
}

void MultithreadedBallsSceneTest::prepareSecondaryCommandBuffer(std::size_t threadIndex,
                                                                std::size_t frameIndex,
                                                                std::size_t rangeFrom,
                                                                std::size_t rangeTo)
{
    TIME_IT("CmdBuffer (secondary) building");

    // Update test state from own range
    updateTestState(static_cast<float>(window().frameTime()), rangeFrom, rangeTo);

    // Update secondary command buffer
    const vk::CommandBuffer& cmdBuffer = _threadCmdPools[threadIndex].cmdBuffers[frameIndex];

    cmdBuffer.reset({});
    vk::CommandBufferInheritanceInfo inheritanceInfo{_renderPass, 0, _framebuffers[frameIndex], VK_FALSE, {}, {}};
    cmdBuffer.begin(vk::CommandBufferBeginInfo{vk::CommandBufferUsageFlagBits::eRenderPassContinue |
                                                   vk::CommandBufferUsageFlagBits::eOneTimeSubmit,
                                               &inheritanceInfo});
    {
        cmdBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, _pipeline);
        cmdBuffer.bindVertexBuffers(0, {{_vbo.buffer}}, {{0}});

        for (std::size_t ballIndex = rangeFrom; ballIndex < rangeTo; ++ballIndex) {
            cmdBuffer.pushConstants(_pipelineLayout, vk::ShaderStageFlagBits::eVertex, 0, sizeof(glm::vec4),
                                    &balls()[ballIndex].position);
            cmdBuffer.pushConstants(_pipelineLayout, vk::ShaderStageFlagBits::eFragment, sizeof(glm::vec4),
                                    sizeof(glm::vec4), &balls()[ballIndex].color);
            cmdBuffer.draw(static_cast<uint32_t>(vertices().size()), 1, 0, 0);
        }
    }
    cmdBuffer.end();
}

void MultithreadedBallsSceneTest::prepareCommandBuffer(std::size_t frameIndex)
{
    static const vk::ClearValue clearValue = vk::ClearColorValue{std::array<float, 4>{{0.0f, 0.0f, 0.0f, 1.0f}}};
    vk::RenderPassBeginInfo renderPassInfo{
        _renderPass, _framebuffers[frameIndex], {{}, {window().size().x, window().size().y}}, 1, &clearValue};

    {
        TIME_IT("Fence waiting");
        device().waitForFences(1, &_fences[frameIndex], VK_FALSE, UINT64_MAX);
        device().resetFences(1, &_fences[frameIndex]);
    }

    {
        TIME_IT("CmdBuffer building");
        const vk::CommandBuffer& cmdBuffer = _cmdBuffers[frameIndex];
        cmdBuffer.reset({});
        cmdBuffer.begin({vk::CommandBufferUsageFlagBits::eOneTimeSubmit, nullptr});
        {
            cmdBuffer.beginRenderPass(renderPassInfo, vk::SubpassContents::eSecondaryCommandBuffers);

            std::vector<std::thread> threads(_threadCmdPools.size());
            std::vector<vk::CommandBuffer> threadedCommandBuffers;
            for (std::size_t threadIndex = 0; threadIndex < _threadCmdPools.size(); ++threadIndex) {
                std::size_t k = balls().size() / _threadCmdPools.size();
                std::size_t rangeFrom = threadIndex * k;
                std::size_t rangeTo = (threadIndex + 1) * k;

                threadedCommandBuffers.push_back(_threadCmdPools[threadIndex].cmdBuffers[frameIndex]);

                threads[threadIndex] =
                    std::move(std::thread(&MultithreadedBallsSceneTest::prepareSecondaryCommandBuffer, this,
                                          threadIndex, frameIndex, rangeFrom, rangeTo));
            }
            for (auto& thread : threads) {
                thread.join();
            }

            cmdBuffer.executeCommands(threadedCommandBuffers);
            cmdBuffer.endRenderPass();
        }
        cmdBuffer.end();
    }
}

void MultithreadedBallsSceneTest::submitCommandBuffer(std::size_t frameIndex)
{
    TIME_IT("CmdBuffer submition");

    vk::PipelineStageFlags waitStage = vk::PipelineStageFlagBits::eColorAttachmentOutput;
    vk::SubmitInfo submits{1, &_acquireSemaphores[_semaphoreIndex], &waitStage, 1, &_cmdBuffers[frameIndex],
                           1, &_renderSemaphores[_semaphoreIndex]};
    queues().queue().submit(submits, _fences[frameIndex]);
}

void MultithreadedBallsSceneTest::presentFrame(std::size_t frameIndex)
{
    TIME_IT("Frame presentation");

    uint32_t imageIndex = static_cast<uint32_t>(frameIndex);
    vk::PresentInfoKHR presentInfo{1,      &_renderSemaphores[_semaphoreIndex], 1, &window().swapchain(), &imageIndex,
                                   nullptr};
    queues().queue().presentKHR(presentInfo);
}
}
}
