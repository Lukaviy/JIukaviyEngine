module;

#include <ranges>
#define GLFW_INCLUDE_VULKAN
#include <glfwpp/glfwpp.h>
#include <glm/glm.hpp>
#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>

#include <spdlog/spdlog.h>
#include <fmt/ranges.h>

export module ApplicationWindows;

import application;
import pointers;
import resource_system;

namespace vk::utils {
	consteval bool isValidationLayersEnabled() {
#ifndef NDEBUG
		return true;
#else
		return false;
#endif
	}

	static constexpr std::array<const char*, 1> validationLayers{
		"VK_LAYER_KHRONOS_validation"
	};

	static constexpr std::array<const char*, 1> deviceExtensions = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME
	};

	struct QueueFamilyIndices {
		std::optional<std::uint32_t> graphicsFamily;
		std::optional<std::uint32_t> presentFamily;

		bool isComplete() const {
			return graphicsFamily && presentFamily;
		}
	};

	QueueFamilyIndices findQueueFamilies(const vk::raii::PhysicalDevice& device, const vk::raii::SurfaceKHR& surface) {
		QueueFamilyIndices result;

		const auto properties = device.getQueueFamilyProperties();

		for (auto i = 0u; i < properties.size() && !result.isComplete(); ++i) {
			const auto& property = properties[i];
			if (property.queueFlags & vk::QueueFlagBits::eGraphics) {
				result.graphicsFamily = i;
			}

			if (device.getSurfaceSupportKHR(i, surface)) {
				result.presentFamily = i;
			}
		}

		return result;
	}

	vk::DebugUtilsMessengerCreateInfoEXT createDebugMessenger() {
		return {
			.messageSeverity = vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose |
				vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
				vk::DebugUtilsMessageSeverityFlagBitsEXT::eError,
			.messageType = vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
				vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation |
				vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance,
			.pfnUserCallback = +[](
				VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
				VkDebugUtilsMessageTypeFlagsEXT message_type,
				const VkDebugUtilsMessengerCallbackDataEXT* p_callback_data,
				void*
			) -> vk::Bool32
			{
				spdlog::info("[Vulkan] [{}] {}", p_callback_data->pMessageIdName, p_callback_data->pMessage);
				return vk::False;
			}
		};
	}

	bool checkValidationSupport() {
		const auto available_layers = vk::enumerateInstanceLayerProperties();

		spdlog::info("Available validation layers: [\n{}\n]", fmt::join(available_layers | std::ranges::views::transform([](const vk::LayerProperties& layer) {
			return std::string_view(layer.layerName.data());
		}), ",\n"));

		for (const auto& validation_layer : validationLayers) {
			if (std::ranges::find_if(available_layers, [&](const vk::LayerProperties& layer) {
				return std::string_view(layer.layerName.data()) == std::string_view(validation_layer);
			}) == available_layers.end()) {
				return false;
			}
		}

		return true;
	}

	vk::raii::Instance createInstance(const ji::ApplicationInfo& info) {
		if (isValidationLayersEnabled() && !checkValidationSupport()) {
			throw std::runtime_error("Validation layers requested, but not available!");
		}

		const vk::raii::Context context;

		const vk::ApplicationInfo application_info{
			.pApplicationName = info.name.data(),
			.applicationVersion = VK_MAKE_VERSION(1, 0, 0),
			.pEngineName = "JIukaviyEngine",
			.engineVersion = VK_MAKE_VERSION(1, 0, 0),
			.apiVersion = VK_API_VERSION_1_3
		};

		auto extensions = glfw::getRequiredInstanceExtensions();

		extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

		vk::InstanceCreateInfo instance_create_info{
			.pApplicationInfo = &application_info,
			.enabledExtensionCount = static_cast<uint32_t>(extensions.size()),
			.ppEnabledExtensionNames = extensions.data(),
		};

		instance_create_info.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
		instance_create_info.ppEnabledLayerNames = validationLayers.data();

		auto instance_create_info_chain = vk::StructureChain{ instance_create_info, createDebugMessenger() };

		return vk::raii::Instance(context, instance_create_info_chain.get<>());
	}

	vk::raii::Device createDevice(
		const vk::raii::PhysicalDevice& physical_device,
		const vk::raii::SurfaceKHR& surface
	) {
		const auto indices = findQueueFamilies(physical_device, surface);

		float queuePriority = 1.0f;

		vk::DeviceQueueCreateInfo queue_create_info{
			.queueFamilyIndex = indices.graphicsFamily.value(),
			.queueCount = 1,
			.pQueuePriorities = &queuePriority
		};

		vk::PhysicalDeviceFeatures device_features{
			/*.geometryShader = vk::True,
			.sampleRateShading = vk::True,
			.multiDrawIndirect = vk::False,*/
		};

		vk::DeviceCreateInfo device_create_info{
			.queueCreateInfoCount = 1,
			.pQueueCreateInfos = &queue_create_info,
			.enabledExtensionCount = 1,
			.ppEnabledExtensionNames = deviceExtensions.data(),
			.pEnabledFeatures = &device_features
		};

		return { physical_device, device_create_info };
	}

	bool checkDeviceExtensionSupport(const vk::PhysicalDevice& physical_device) {
		const auto extenstion_properties = physical_device.enumerateDeviceExtensionProperties();

		for (const auto& device_extension : deviceExtensions) {
			const auto extension_it = std::ranges::find_if(extenstion_properties, [&](const ExtensionProperties& property) {
				return std::string_view(device_extension) == std::string_view(property.extensionName.data());
			});

			if (extension_it == extenstion_properties.end()) {
				return false;
			}
		}

		return true;
	}

	struct SwapChainSupportDetails {
		vk::SurfaceCapabilitiesKHR capabilities;
		std::vector<vk::SurfaceFormatKHR> formats;
		std::vector<vk::PresentModeKHR> presentModes;
	};

	SwapChainSupportDetails getChainSupportDetails(const vk::raii::PhysicalDevice& device, const vk::raii::SurfaceKHR& surface) {
		return {
			.capabilities = device.getSurfaceCapabilitiesKHR(surface),
			.formats = device.getSurfaceFormatsKHR(surface),
			.presentModes = device.getSurfacePresentModesKHR(surface)
		};
	}

	bool checkSwapChainSupport(const SwapChainSupportDetails& support_details) {
		return !support_details.formats.empty() && !support_details.presentModes.empty();
	}

	vk::SurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& formats) {
		if (formats.empty()) {
			throw std::runtime_error("Available formats for surface is empty!");
		}

		for (const auto& format : formats) {
			if (format.format == vk::Format::eB8G8R8Srgb && format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear) {
				return format;
			}
		}

		return formats.front();
	}

	vk::PresentModeKHR chooseSwapPresentMode(const std::vector<vk::PresentModeKHR>& present_modes) {
		if (present_modes.empty()) {
			throw std::runtime_error("Available present modes for surface is empty!");
		}

		const auto it = std::ranges::find(present_modes, vk::PresentModeKHR::eMailbox);
		if (it != present_modes.end()) {
			return *it;
		}

		return vk::PresentModeKHR::eFifo;
	}

	vk::Extent2D chooseSwapExtent(const glfw::Window& window, const vk::SurfaceCapabilitiesKHR& capabilities) {
		if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
			return capabilities.currentExtent;
		}

		const auto [width, height] = window.getFramebufferSize();

		return {
			.width = std::clamp(static_cast<uint32_t>(width), capabilities.minImageExtent.width, capabilities.maxImageExtent.width),
			.height = std::clamp(static_cast<uint32_t>(height), capabilities.minImageExtent.height, capabilities.maxImageExtent.height)
		};
	}

	bool isDeviceSuitable(const vk::raii::PhysicalDevice& device, const vk::raii::SurfaceKHR& surface) {
		return
			device.getProperties().deviceType == vk::PhysicalDeviceType::eDiscreteGpu &&
			device.getFeatures().geometryShader &&
			findQueueFamilies(device, surface).isComplete() &&
			checkDeviceExtensionSupport(device) &&
			checkSwapChainSupport(getChainSupportDetails(device, surface))
		;
	}

	void logPhysicalDevicesInfo(const vk::raii::PhysicalDevices& physical_devices) {
		spdlog::info("Available physical devices: \n{}", fmt::join(physical_devices | std::ranges::views::transform([](const vk::raii::PhysicalDevice& device) {
			return fmt::format("Device Name: {}, API version: {}", 
				device.getProperties().deviceName.data(), 
				device.getProperties().apiVersion
			);
		}), "\n"));
	}

	vk::raii::PhysicalDevice selectPhysicalDevice(const vk::raii::PhysicalDevices& physical_devices, const vk::raii::SurfaceKHR& surface) {
		if (physical_devices.empty()) {
			throw std::runtime_error("No Vulkan physical devices found!");
		}

		const auto physical_device = std::ranges::find_if(physical_devices, [&](const vk::raii::PhysicalDevice& device) {
			return isDeviceSuitable(device, surface);
		});

		if (physical_device == physical_devices.end()) {
			throw std::runtime_error("No suitable Vulkan physical device found!");
		}

		return *physical_device;
	}

	vk::raii::Queue createQueue(
		const vk::raii::Device& device,
		const QueueFamilyIndices& indices
	) {
		if (!indices.graphicsFamily.has_value()) {
			throw std::runtime_error("No graphics queue family found!");
		}
		return { device, indices.graphicsFamily.value(), 0 };
	}

	vk::raii::SurfaceKHR createSurface(
		const vk::raii::Instance& instance,
		const glfw::Window& window
	) {
		VkSurfaceKHR surface;
		if (glfwCreateWindowSurface(*instance, window, nullptr, &surface) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create Vulkan surface!");
		}
		return vk::raii::SurfaceKHR{ instance, surface };
	}

	glfw::Window createWindow(const ji::ApplicationInfo& app_info) {
		const glfw::WindowHints window_hints{
			.clientApi = glfw::ClientApi::None
		};

		window_hints.apply();

		return { 800, 600, app_info.name.data() };
	}

	vk::SwapchainCreateInfoKHR getSwapChainCreateInfo(const SwapChainSupportDetails& swap_chain_support_details, const glfw::Window& window, const vk::raii::Device& device, const vk::raii::SurfaceKHR& surface, const QueueFamilyIndices& queue_family_indices) {
		const auto image_count = [&]() {
			const auto desired = swap_chain_support_details.capabilities.minImageCount + 1;
			if (swap_chain_support_details.capabilities.maxImageCount > 0) {
				return std::min(desired, swap_chain_support_details.capabilities.maxImageCount);
			}
			return desired;
		}();

		const auto surface_format = chooseSwapSurfaceFormat(swap_chain_support_details.formats);
		const auto present_mode = chooseSwapPresentMode(swap_chain_support_details.presentModes);
		const auto extent = chooseSwapExtent(window, swap_chain_support_details.capabilities);

		const auto diff_indices = queue_family_indices.graphicsFamily != queue_family_indices.presentFamily;
		const auto indices = std::array{ queue_family_indices.graphicsFamily.value(), queue_family_indices.presentFamily.value() };

		return vk::SwapchainCreateInfoKHR{
			.surface = surface,
			.minImageCount = image_count,
			.imageFormat = surface_format.format,
			.imageColorSpace = surface_format.colorSpace,
			.imageExtent = extent,
			.imageArrayLayers = 1,
			.imageUsage = vk::ImageUsageFlagBits::eColorAttachment,
			.imageSharingMode = diff_indices ? vk::SharingMode::eConcurrent : vk::SharingMode::eExclusive,
			.queueFamilyIndexCount = diff_indices ? static_cast<uint32_t>(indices.size()) : 0u,
			.pQueueFamilyIndices = diff_indices ? indices.data() : nullptr,
			.preTransform = swap_chain_support_details.capabilities.currentTransform,
			.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque,
			.presentMode = present_mode,
			.clipped = vk::True,
			.oldSwapchain = nullptr
		};
	}

	std::vector<vk::raii::ImageView> createImageViews(const vk::raii::Device& device, const vk::raii::SwapchainKHR& swap_chain, const vk::SwapchainCreateInfoKHR& swap_chain_create_info) {
		const auto images = swap_chain.getImages();

		std::vector<vk::raii::ImageView> res;
		res.reserve(images.size());

		for (const auto& image : images) {
			const auto create_info = vk::ImageViewCreateInfo{
				.image = image,
				.viewType = vk::ImageViewType::e2D,
				.format = swap_chain_create_info.imageFormat,
				.components = {
					.r = ComponentSwizzle::eIdentity,
					.g = ComponentSwizzle::eIdentity,
					.b = ComponentSwizzle::eIdentity,
					.a = ComponentSwizzle::eIdentity,
				},
				.subresourceRange = {
					.aspectMask = vk::ImageAspectFlagBits::eColor,
					.baseMipLevel = 0,
					.levelCount = 1,
					.baseArrayLayer = 0,
					.layerCount = 1
				}
			};

			res.push_back(device.createImageView(create_info));
		}

		return res;
	}

	vk::raii::ShaderModule createShaderModule(const vk::raii::Device& device, const std::vector<std::byte>& code) {
		vk::ShaderModuleCreateInfo create_info{
			.codeSize = code.size(),
			.pCode = reinterpret_cast<const uint32_t*>(code.data())
		};
		return { device, create_info };
	}

	vk::raii::RenderPass createRenderPass(const vk::raii::Device& device, const vk::Format& format) {
		const auto color_attachment = vk::AttachmentDescription{
			.format = format,
			.samples = SampleCountFlagBits::e1,
			.loadOp = AttachmentLoadOp::eClear,
			.storeOp = AttachmentStoreOp::eStore,
			.stencilLoadOp = AttachmentLoadOp::eDontCare,
			.stencilStoreOp = AttachmentStoreOp::eDontCare,
			.initialLayout = ImageLayout::eUndefined,
			.finalLayout = ImageLayout::ePresentSrcKHR
		};

		const auto attachment_reference = vk::AttachmentReference{
			.attachment = 0,
			.layout = ImageLayout::eColorAttachmentOptimal
		};

		const auto subpass = vk::SubpassDescription{
			.pipelineBindPoint = PipelineBindPoint::eGraphics,
			.colorAttachmentCount = 1,
			.pColorAttachments = &attachment_reference
		};

		const auto render_pass_create_info = vk::RenderPassCreateInfo{
			.attachmentCount = 1,
			.pAttachments = &color_attachment,
			.subpassCount = 1,
			.pSubpasses = &subpass
		};

		return device.createRenderPass(render_pass_create_info);
	}

	vk::raii::PipelineLayout createGraphicsPipelineLayout(const vk::raii::Device& device) {
		return device.createPipelineLayout(vk::PipelineLayoutCreateInfo{
			.setLayoutCount = 0,
			.pSetLayouts = nullptr,
			.pushConstantRangeCount = 0,
			.pPushConstantRanges = nullptr
		});
	}

	vk::raii::Pipeline createGraphicsPipeline(const vk::raii::Device& device, const vk::Extent2D& swapchain_extent, const vk::raii::PipelineLayout& pipeline_layout, const vk::raii::RenderPass& render_pass) {
		const auto vert_shader = ji::ResourceSystem::loadFile("res/shaders/shader.vert.spv");
		const auto frag_shader = ji::ResourceSystem::loadFile("res/shaders/shader.frag.spv");

		const auto vert_shader_module = createShaderModule(device, vert_shader);
		const auto frag_shader_module = createShaderModule(device, frag_shader);

		const auto vert_shader_stage_info = vk::PipelineShaderStageCreateInfo{
			.stage = vk::ShaderStageFlagBits::eVertex,
			.module = vert_shader_module,
			.pName = "main"
		};

		const auto frag_shader_stage_info = vk::PipelineShaderStageCreateInfo{
			.stage = vk::ShaderStageFlagBits::eFragment,
			.module = frag_shader_module,
			.pName = "main"
		};

		const auto shader_stages = std::array{ vert_shader_stage_info, frag_shader_stage_info };

		const auto vertex_input_create_info = vk::PipelineVertexInputStateCreateInfo{
			.vertexBindingDescriptionCount = 0,
			.vertexAttributeDescriptionCount = 0
		};

		const auto input_assembly_create_info = vk::PipelineInputAssemblyStateCreateInfo{
			.topology = PrimitiveTopology::eTriangleList,
			.primitiveRestartEnable = false
		};

		const auto viewport = vk::Viewport{
			.x = 0.f,
			.y = 0.f,
			.width = static_cast<float>(swapchain_extent.width),
			.height = static_cast<float>(swapchain_extent.height),
			.minDepth = 0.f,
			.maxDepth = 1.f
		};

		const auto scissor = vk::Rect2D{
			.offset = {0, 0},
			.extent = swapchain_extent
		};

		const auto dynamic_states = std::vector{
			vk::DynamicState::eViewport,
			vk::DynamicState::eScissor
		};

		const auto dynamic_state_create_info = vk::PipelineDynamicStateCreateInfo{
			.dynamicStateCount = static_cast<uint32_t>(dynamic_states.size()),
			.pDynamicStates = dynamic_states.data()
		};

		const auto viewport_state_create_info = vk::PipelineViewportStateCreateInfo{
			.viewportCount = 1,
			.pViewports = &viewport,
			.scissorCount = 1,
			.pScissors = &scissor
		};

		const auto rasterization_create_info = vk::PipelineRasterizationStateCreateInfo{
			.depthClampEnable = vk::False,
			.rasterizerDiscardEnable = vk::False,
			.polygonMode = PolygonMode::eFill,
			.cullMode = CullModeFlagBits::eBack,
			.frontFace = FrontFace::eClockwise,
			.depthBiasEnable = vk::False,
			.depthBiasConstantFactor = 0.f,
			.depthBiasClamp = 0.f,
			.depthBiasSlopeFactor = 0.f,
			.lineWidth = 1.f
		};

		const auto multisample_state_create_info = vk::PipelineMultisampleStateCreateInfo{
			.rasterizationSamples = SampleCountFlagBits::e1,
			.sampleShadingEnable = vk::False,
			.minSampleShading = 1.f,
			.pSampleMask = nullptr,
			.alphaToCoverageEnable = vk::False,
			.alphaToOneEnable = vk::False
		};

		const auto color_blend_attachment = vk::PipelineColorBlendAttachmentState{
			.blendEnable = vk::False,
			.srcColorBlendFactor = BlendFactor::eOne,
			.dstColorBlendFactor = BlendFactor::eZero,
			.colorBlendOp = BlendOp::eAdd,
			.srcAlphaBlendFactor = BlendFactor::eOne,
			.dstAlphaBlendFactor = BlendFactor::eZero,
			.alphaBlendOp = BlendOp::eAdd,
			.colorWriteMask = ColorComponentFlagBits::eR | ColorComponentFlagBits::eG | ColorComponentFlagBits::eB | ColorComponentFlagBits::eA
		};

		const auto color_blend_state_create_info = vk::PipelineColorBlendStateCreateInfo{
			.logicOpEnable = vk::False,
			.logicOp = LogicOp::eCopy,
			.attachmentCount = 1,
			.pAttachments = &color_blend_attachment,
			.blendConstants = std::array{
				0.f,
				0.f,
				0.f,
				0.f
			}
		};

		const auto pipeline_info = vk::GraphicsPipelineCreateInfo{
			.stageCount = shader_stages.size(),
			.pStages = shader_stages.data(),
			.pVertexInputState = &vertex_input_create_info,
			.pInputAssemblyState = &input_assembly_create_info,
			.pViewportState = &viewport_state_create_info,
			.pRasterizationState = &rasterization_create_info,
			.pMultisampleState = &multisample_state_create_info,
			.pDepthStencilState = nullptr,
			.pColorBlendState = &color_blend_state_create_info,
			.pDynamicState = &dynamic_state_create_info,
			.layout = pipeline_layout,
			.renderPass = render_pass,
			.subpass = 0,
			.basePipelineHandle = nullptr,
			.basePipelineIndex = -1
		};

		return device.createGraphicsPipeline(nullptr, pipeline_info);
	}
}

namespace ji {
	class ApplicationWindows : public Application {
	public:
		void run() override {
			while (!m_window.shouldClose()) {
				glfw::pollEvents();
			}
		}

		ApplicationWindows(
			glfw::GlfwLibrary lib,
			ApplicationInfo&& info,
			vk::raii::Instance&& instance,
			vk::raii::Device&& device,
			vk::raii::Queue&& queue,
			glfw::Window&& window,
			vk::raii::SurfaceKHR&& surface,
			vk::raii::SwapchainKHR&& swap_chain,
			std::vector<vk::raii::ImageView>&& image_views,
			vk::raii::PipelineLayout&& pipeline_layout,
			vk::raii::RenderPass&& render_pass,
			vk::raii::Pipeline&& graphics_pipeline
		) :
			m_vkInstance(std::move(instance)),
			m_glfw(std::move(lib)),
			m_window(std::move(window)),
			m_applicationInfo(std::move(info)),
			m_vkDevice(std::move(device)),
			m_vkQueue(std::move(queue)),
			m_vkSurface(std::move(surface)),
			m_vkSwapChain(std::move(swap_chain)),
			m_vkImageViews(std::move(image_views)),
			m_pipelineLayout(std::move(pipeline_layout)),
			m_renderPass(std::move(render_pass)),
			m_graphicsPipeline(std::move(graphics_pipeline))
		{
#ifndef NDEBUG
			m_debugMessenger = vk::raii::DebugUtilsMessengerEXT{ m_vkInstance, vk::utils::createDebugMessenger() };
#endif
		}

	private:
		// Order is important here, it declares in what order destructors will be called

		vk::raii::Instance m_vkInstance;
		glfw::GlfwLibrary m_glfw;
		glfw::Window m_window;
		ApplicationInfo m_applicationInfo;
		vk::raii::Device m_vkDevice;
		vk::raii::Queue m_vkQueue;
		vk::raii::SurfaceKHR m_vkSurface;
		vk::raii::SwapchainKHR m_vkSwapChain;
		std::vector<vk::raii::ImageView> m_vkImageViews;
		std::optional<vk::raii::DebugUtilsMessengerEXT> m_debugMessenger;
		vk::raii::PipelineLayout m_pipelineLayout;
		vk::raii::RenderPass m_renderPass;
		vk::raii::Pipeline m_graphicsPipeline;
	};

	ji::unique<Application> Application::create(ApplicationInfo&& info) {
		auto lib = glfw::init();
		auto instance = vk::utils::createInstance(info);
		auto window = vk::utils::createWindow(info);
		auto surface = vk::utils::createSurface(instance, window);
		const auto physical_devices = vk::raii::PhysicalDevices{ instance };
		vk::utils::logPhysicalDevicesInfo(physical_devices);
		const auto physical_device = vk::utils::selectPhysicalDevice(physical_devices, surface);
		auto device = vk::utils::createDevice(physical_device, surface);
		const auto queue_family_indices = vk::utils::findQueueFamilies(physical_device, surface);
		auto queue = vk::utils::createQueue(device, queue_family_indices);
		const auto swap_chain_create_info = vk::utils::getSwapChainCreateInfo(vk::utils::getChainSupportDetails(physical_device, surface), window, device, surface, queue_family_indices);
		auto swap_chain = vk::raii::SwapchainKHR{ device, swap_chain_create_info };
		auto image_views = vk::utils::createImageViews(device, swap_chain, swap_chain_create_info);
		auto pipeline_layout = vk::utils::createGraphicsPipelineLayout(device);
		auto render_pass = vk::utils::createRenderPass(device, swap_chain_create_info.imageFormat);
		auto graphics_pipeline = vk::utils::createGraphicsPipeline(device, swap_chain_create_info.imageExtent, pipeline_layout, render_pass);

		return ji::make_unique<ApplicationWindows>(
			std::move(lib),
			std::move(info),
			std::move(instance),
			std::move(device),
			std::move(queue),
			std::move(window),
			std::move(surface),
			std::move(swap_chain),
			std::move(image_views),
			std::move(pipeline_layout),
			std::move(render_pass),
			std::move(graphics_pipeline)
		);
	}
}
