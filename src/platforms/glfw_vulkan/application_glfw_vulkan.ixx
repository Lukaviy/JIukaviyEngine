module;

#include <ranges>
#include <array>
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

		const auto subpass_dependency = vk::SubpassDependency{
			.srcSubpass = vk::SubpassExternal,
			.dstSubpass = 0,
			.srcStageMask = PipelineStageFlagBits::eColorAttachmentOutput,
			.dstStageMask = PipelineStageFlagBits::eColorAttachmentOutput,
			.srcAccessMask = vk::AccessFlags{},
			.dstAccessMask = AccessFlagBits::eColorAttachmentWrite
		};

		const auto render_pass_create_info = vk::RenderPassCreateInfo{
			.attachmentCount = 1,
			.pAttachments = &color_attachment,
			.subpassCount = 1,
			.pSubpasses = &subpass,
			.dependencyCount = 1,
			.pDependencies = &subpass_dependency
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

	struct Vertex {
		glm::vec2 pos;
		glm::vec3 color;

		static vk::VertexInputBindingDescription getBindingDescription() {
			return vk::VertexInputBindingDescription{
				.binding = 0,
				.stride = sizeof(Vertex),
				.inputRate = vk::VertexInputRate::eVertex
			};
		}

		static std::array<vk::VertexInputAttributeDescription, 2> getAttributeDescription() {
			return std::array{
				vk::VertexInputAttributeDescription{
					.location = 0,
					.binding = 0,
					.format = vk::Format::eR32G32Sfloat,
					.offset = offsetof(Vertex, pos)
				},
				vk::VertexInputAttributeDescription{
					.location = 1,
					.binding = 0,
					.format = vk::Format::eR32G32B32Sfloat,
					.offset = offsetof(Vertex, color)
				}
			};
		}
	};

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

		const auto vertex_binding_description = Vertex::getBindingDescription();
		const auto vertex_attribute_descriptions = Vertex::getAttributeDescription();

		const auto vertex_input_create_info = vk::PipelineVertexInputStateCreateInfo{
			.vertexBindingDescriptionCount = 1,
			.pVertexBindingDescriptions = &vertex_binding_description,
			.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertex_attribute_descriptions.size()),
			.pVertexAttributeDescriptions = vertex_attribute_descriptions.data()
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

		return device.createGraphicsPipeline(nullptr, {
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
		});
	}

	std::vector<vk::raii::Framebuffer> createFrameBuffers(const vk::raii::Device& device, const vk::raii::RenderPass& render_pass, const vk::Extent2D& swap_chain_extent, std::span<const vk::raii::ImageView> image_views) {
		std::vector<vk::raii::Framebuffer> framebuffers;
		framebuffers.reserve(image_views.size());

		for (const auto& image_view : image_views) {
			const auto attachments = std::array{
				*image_view
			};

			framebuffers.emplace_back(device.createFramebuffer({
				.renderPass = render_pass,
				.attachmentCount = static_cast<uint32_t>(attachments.size()),
				.pAttachments = attachments.data(),
				.width = swap_chain_extent.width,
				.height = swap_chain_extent.height,
				.layers = 1
			}));
		}

		return framebuffers;
	}

	vk::raii::CommandPool createCommandPool(const vk::raii::Device& device, const QueueFamilyIndices& queue_family_indices) {
		return device.createCommandPool(vk::CommandPoolCreateInfo{
			.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
			.queueFamilyIndex = queue_family_indices.graphicsFamily.value()
		});
	}

	vk::raii::CommandBuffer createCommandBuffer(const vk::raii::Device& device, const vk::raii::CommandPool& command_pool) {
		auto command_buffers = device.allocateCommandBuffers(vk::CommandBufferAllocateInfo{
			.commandPool = command_pool,
			.level = vk::CommandBufferLevel::ePrimary,
			.commandBufferCount = 1
		});
		return std::move(command_buffers.front());
	}

	std::uint32_t findMemoryType(std::uint32_t type_filter, vk::MemoryPropertyFlags properties, const vk::PhysicalDeviceMemoryProperties& memory_properties) {
		for (auto i = 0u; i < memory_properties.memoryTypeCount; ++i) {
			if ((type_filter & (1 << i)) && (memory_properties.memoryTypes[i].propertyFlags & properties) == properties) {
				return i;
			}
		}

		throw std::runtime_error("Failed to find suitable memory type!");
	}

	std::tuple<vk::raii::Buffer, vk::raii::DeviceMemory> createBuffer(const vk::raii::Device& device, const vk::raii::PhysicalDevice& physical_device, BufferUsageFlags usage, const vk::MemoryPropertyFlags memory_property_flags, vk::DeviceSize size) {
		auto buffer = device.createBuffer({
			.size = size,
			.usage = usage,
			.sharingMode = SharingMode::eExclusive
		});

		const auto memory_requirements = buffer.getMemoryRequirements();

		const auto memory_type = vk::utils::findMemoryType(
			memory_requirements.memoryTypeBits,
			memory_property_flags,
			physical_device.getMemoryProperties()
		);

		auto vertex_buffer_memory = device.allocateMemory({
			.allocationSize = memory_requirements.size,
			.memoryTypeIndex = memory_type
		});

		buffer.bindMemory(vertex_buffer_memory, 0);

		return {
			std::move(buffer),
			std::move(vertex_buffer_memory)
		};
	}

	void copyBuffer(const vk::raii::Device& device, const vk::raii::CommandPool& command_pool, const vk::raii::Queue& queue, const vk::Buffer& src, const vk::Buffer& dst, vk::DeviceSize size) {
		const auto commands_buffers = device.allocateCommandBuffers({
			.commandPool = command_pool,
			.commandBufferCount = 1
		});

		const auto& cb = commands_buffers.front();

		cb.begin({
			.flags = CommandBufferUsageFlagBits::eOneTimeSubmit
		});

		cb.copyBuffer(src, dst, std::array{
			vk::BufferCopy{
				.srcOffset = 0,
				.dstOffset = 0,
				.size = size
			}
		});

		cb.end();

		const auto submit_command_buffers = std::array{ *cb };

		queue.submit(std::array{
			vk::SubmitInfo{
				.commandBufferCount = static_cast<uint32_t>(submit_command_buffers.size()),
				.pCommandBuffers = submit_command_buffers.data()
			}
		}, {});

		queue.waitIdle();
	}
}

namespace ji {
	class ApplicationWindows : public Application {
	public:
		void run() override {
			while (!m_context.window.shouldClose()) {
				glfw::pollEvents();
				drawFrame();
			}

			m_context.device.waitIdle();
		}

		struct Context {
			// Order is important here, it declares in what order destructors will be called
			vk::raii::Instance instance;
			glfw::GlfwLibrary glfw;
			glfw::Window window;
			ApplicationInfo applicationInfo;
			vk::raii::Device device;
			vk::raii::PhysicalDevice physicalDevice;
			vk::raii::Queue queue;
			vk::raii::SurfaceKHR surface;
			vk::raii::PipelineLayout pipelineLayout;
			vk::raii::RenderPass renderPass;
			vk::raii::Pipeline graphicsPipeline;
			vk::raii::CommandPool commandPool;
			std::optional<vk::raii::DebugUtilsMessengerEXT> debugMessenger;

			struct SwapChainObject {
				vk::raii::SwapchainKHR swapChain;
				vk::Extent2D extent;
				std::vector<vk::raii::ImageView> imageViews;
				std::vector<vk::raii::Framebuffer> frameBuffers;
			} swapChainObject;

			struct SyncObjects {
				vk::raii::Semaphore imageAvailableSemaphore;
				vk::raii::Semaphore renderFinishedSemaphore;
				vk::raii::Fence inFlightFence;
			};

			struct FrameObject {
				vk::raii::CommandBuffer commandBuffer;
				SyncObjects m_syncObjects;
			};

			std::vector<FrameObject> frameObjects;
		};

		struct Buffers {
			vk::raii::Buffer indexBuffer;
			vk::raii::DeviceMemory indexBufferMemory;
			vk::raii::Buffer vertexBuffer;
			vk::raii::DeviceMemory vertexBufferMemory;
		};

		ApplicationWindows(ApplicationInfo&& info) :
			m_context(createContext(std::move(info))),
			m_buffers(createBuffers(m_context.device, m_context.physicalDevice, m_context.commandPool, m_context.queue, m_vertices, m_indices))
		{
#ifndef NDEBUG
			m_context.debugMessenger = vk::raii::DebugUtilsMessengerEXT{ m_context.instance, vk::utils::createDebugMessenger() };
#endif

			m_context.window.sizeEvent.setCallback([this](glfw::Window&, int, int) {
				m_swapChainDirty = true;
			});
		}

	private:
		Context m_context;
		size_t m_currentFrame{};
		bool m_swapChainDirty{};
		static constexpr size_t g_MaxFramesInFlight = 2;

		const std::vector<vk::utils::Vertex> m_vertices = {
			{{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}},
			{{0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},
			{{0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}},
			{{-0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}}
		};

		const std::vector<std::uint16_t> m_indices = {
			0, 1, 2, 2, 3, 0
		};

		Buffers m_buffers;

		void recordCommandBuffer(const Context::FrameObject& frame_object, const vk::raii::Framebuffer& frame_buffer) const {
			const auto& cb = frame_object.commandBuffer;

			cb.begin({});

			const auto clear_color = vk::ClearValue{
				vk::ClearColorValue{
					std::array{0.f, 0.f, 0.f, 1.f}
				}
			};
			cb.beginRenderPass({
				.renderPass = m_context.renderPass,
				.framebuffer = frame_buffer,
				.renderArea = {
					.offset = {0, 0},
					.extent = m_context.swapChainObject.extent
				},
				.clearValueCount = 1,
				.pClearValues = &clear_color
			}, vk::SubpassContents::eInline);

			cb.bindPipeline(vk::PipelineBindPoint::eGraphics, m_context.graphicsPipeline);

			cb.setViewport(0, std::array{
				vk::Viewport{
					.x = 0.f,
					.y = 0.f,
					.width = static_cast<float>(m_context.swapChainObject.extent.width),
					.height = static_cast<float>(m_context.swapChainObject.extent.height),
					.minDepth = 0.f,
					.maxDepth = 1.f
				}
			});

			cb.setScissor(0, std::array{
				vk::Rect2D{
					.offset = {0, 0},
					.extent = m_context.swapChainObject.extent
				}
			});

			cb.bindVertexBuffers(0, std::array{ *m_buffers.vertexBuffer }, std::array{ vk::DeviceSize{0} });
			cb.bindIndexBuffer(m_buffers.indexBuffer, 0, vk::IndexType::eUint16);

			cb.drawIndexed(static_cast<uint32_t>(m_indices.size()), 1, 0, 0, 0);

			cb.endRenderPass();

			cb.end();
		}

		void drawFrame() {
			drawFrame(m_context.frameObjects[m_currentFrame]);
			m_currentFrame = (m_currentFrame + 1) % g_MaxFramesInFlight;
		}

		void drawFrame(const Context::FrameObject& frame_object) {
			{
				const auto result = m_context.device.waitForFences(std::array{ *frame_object.m_syncObjects.inFlightFence }, vk::True, std::numeric_limits<std::uint64_t>::max());
			}

			if (m_swapChainDirty) {
				recreateSwapChain();
				m_swapChainDirty = false;
			}

			{
				const auto [acquire_result, image_index] = m_context.device.acquireNextImage2KHR({
					.swapchain = m_context.swapChainObject.swapChain,
					.timeout = std::numeric_limits<std::uint64_t>::max(),
					.semaphore = frame_object.m_syncObjects.imageAvailableSemaphore,
					.deviceMask = 1
				});

				if (acquire_result == vk::Result::eErrorOutOfDateKHR) {
					recreateSwapChain();
					return;
				} 
				if (acquire_result != vk::Result::eSuccess && acquire_result != vk::Result::eSuboptimalKHR) {
					throw std::runtime_error("Failed to acquire swap chain image!");
				}

				m_context.device.resetFences(std::array{ *frame_object.m_syncObjects.inFlightFence });

				frame_object.commandBuffer.reset();

				recordCommandBuffer(frame_object, m_context.swapChainObject.frameBuffers[image_index]);

				const auto wait_semaphores = std::array{ *frame_object.m_syncObjects.imageAvailableSemaphore };
				const auto wait_stages = std::array<vk::PipelineStageFlags, 1>{ vk::PipelineStageFlagBits::eColorAttachmentOutput };
				const auto signal_semaphores = std::array{ *frame_object.m_syncObjects.renderFinishedSemaphore };

				m_context.queue.submit(std::array{
					vk::SubmitInfo{
						.waitSemaphoreCount = static_cast<uint32_t>(wait_semaphores.size()),
						.pWaitSemaphores = wait_semaphores.data(),
						.pWaitDstStageMask = wait_stages.data(),
						.commandBufferCount = 1,
						.pCommandBuffers = &*frame_object.commandBuffer,
						.signalSemaphoreCount = static_cast<uint32_t>(signal_semaphores.size()),
						.pSignalSemaphores = signal_semaphores.data()
					}
				}, frame_object.m_syncObjects.inFlightFence);

				const auto present_result = m_context.queue.presentKHR({
					.waitSemaphoreCount = static_cast<uint32_t>(signal_semaphores.size()),
					.pWaitSemaphores = signal_semaphores.data(),
					.swapchainCount = 1,
					.pSwapchains = &*m_context.swapChainObject.swapChain,
					.pImageIndices = &image_index
				});

				if (present_result == vk::Result::eSuboptimalKHR) {
					recreateSwapChain();
				}
				else if (present_result != vk::Result::eSuccess) {
					throw std::runtime_error("Failed to present swap chain image!");
				}
			}
		}

		void recreateSwapChain() {
			while (true) {
				const auto [width, height] = m_context.window.getFramebufferSize();
				if (width != 0 && height != 0) {
					break;
				}
				glfw::waitEvents();
			};

			m_context.device.waitIdle();

			std::destroy_at(&m_context.swapChainObject);
			std::construct_at(&m_context.swapChainObject, createSwapChainObject(
				m_context.device,
				m_context.renderPass,
				vk::utils::getSwapChainCreateInfo(
					vk::utils::getChainSupportDetails(m_context.physicalDevice, m_context.surface),
					m_context.window,
					m_context.device,
					m_context.surface,
					vk::utils::findQueueFamilies(m_context.physicalDevice, m_context.surface)
				)
			));
		}

		static Context::FrameObject createFrameObject(const vk::raii::Device& device, const vk::raii::CommandPool& command_pool) {
			return {
				.commandBuffer = vk::utils::createCommandBuffer(device, command_pool),
				.m_syncObjects = Context::SyncObjects{
					.imageAvailableSemaphore = device.createSemaphore({}),
					.renderFinishedSemaphore = device.createSemaphore({}),
					.inFlightFence = device.createFence({
						.flags = vk::FenceCreateFlagBits::eSignaled
					})
				}
			};
		}

		static Context::SwapChainObject createSwapChainObject(const vk::raii::Device& device, const vk::raii::RenderPass& render_pass, const vk::SwapchainCreateInfoKHR& swap_chain_create_info) {
			auto swap_chain = vk::raii::SwapchainKHR{ device, swap_chain_create_info };
			auto image_views = vk::utils::createImageViews(device, swap_chain, swap_chain_create_info);
			auto frame_buffers = vk::utils::createFrameBuffers(device, render_pass, swap_chain_create_info.imageExtent, image_views);

			return {
				.swapChain = std::move(swap_chain),
				.extent = swap_chain_create_info.imageExtent,
				.imageViews = std::move(image_views),
				.frameBuffers = std::move(frame_buffers)
			};
		}

		template<class T>
		static std::tuple<vk::raii::Buffer, vk::raii::DeviceMemory> createBuffer(const vk::raii::Device& device, const vk::raii::PhysicalDevice& physical_device, const vk::raii::CommandPool& command_pool, const vk::raii::Queue& queue, vk::BufferUsageFlags usage_flags, std::span<T> data) {
			const auto buffer_size = static_cast<vk::DeviceSize>(sizeof(T) * data.size());

			auto [staging_buffer, staging_buffer_memory] = vk::utils::createBuffer(
				device,
				physical_device,
				vk::BufferUsageFlagBits::eTransferSrc,
				vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
				buffer_size
			);

			const auto mapped_data = staging_buffer_memory.mapMemory(0, buffer_size);
			std::memcpy(mapped_data, data.data(), buffer_size);
			staging_buffer_memory.unmapMemory();

			auto [buffer, buffer_memory] = vk::utils::createBuffer(
				device,
				physical_device,
				vk::BufferUsageFlagBits::eTransferDst | usage_flags,
				vk::MemoryPropertyFlagBits::eDeviceLocal,
				buffer_size
			);

			vk::utils::copyBuffer(device, command_pool, queue, staging_buffer, buffer, buffer_size);

			return {
				std::move(buffer),
				std::move(buffer_memory)
			};
		}

		static Buffers createBuffers(const vk::raii::Device& device, const vk::raii::PhysicalDevice& physical_device, const vk::raii::CommandPool& command_pool, const vk::raii::Queue& queue, std::span<const vk::utils::Vertex> vertices, std::span<const std::uint16_t> indices) {
			auto [vertex_buffer, vertex_buffer_memory] = createBuffer(device, physical_device, command_pool, queue, vk::BufferUsageFlagBits::eVertexBuffer, vertices);
			auto [index_buffer, index_buffer_memory] = createBuffer(device, physical_device, command_pool, queue, vk::BufferUsageFlagBits::eIndexBuffer, indices);

			return {
				.indexBuffer = std::move(index_buffer),
				.indexBufferMemory = std::move(index_buffer_memory),
				.vertexBuffer = std::move(vertex_buffer),
				.vertexBufferMemory = std::move(vertex_buffer_memory)
			};
		}

		static Context createContext(ApplicationInfo&& info) {
			auto lib = glfw::init();
			auto instance = vk::utils::createInstance(info);
			auto window = vk::utils::createWindow(info);
			auto surface = vk::utils::createSurface(instance, window);
			const auto physical_devices = vk::raii::PhysicalDevices{ instance };
			vk::utils::logPhysicalDevicesInfo(physical_devices);
			auto physical_device = vk::utils::selectPhysicalDevice(physical_devices, surface);
			auto device = vk::utils::createDevice(physical_device, surface);
			const auto queue_family_indices = vk::utils::findQueueFamilies(physical_device, surface);
			auto queue = vk::utils::createQueue(device, queue_family_indices);
			const auto swap_chain_create_info = vk::utils::getSwapChainCreateInfo(vk::utils::getChainSupportDetails(physical_device, surface), window, device, surface, queue_family_indices);
			auto pipeline_layout = vk::utils::createGraphicsPipelineLayout(device);
			auto render_pass = vk::utils::createRenderPass(device, swap_chain_create_info.imageFormat);
			auto graphics_pipeline = vk::utils::createGraphicsPipeline(device, swap_chain_create_info.imageExtent, pipeline_layout, render_pass);
			auto command_pool = vk::utils::createCommandPool(device, queue_family_indices);
			auto swap_chain_object = createSwapChainObject(device, render_pass, swap_chain_create_info);

			auto frame_objects = std::vector<Context::FrameObject>{};
			frame_objects.reserve(g_MaxFramesInFlight);
			for (size_t i = 0; i < g_MaxFramesInFlight; i++) {
				frame_objects.push_back(createFrameObject(device, command_pool));
			}

			return {
				.instance = std::move(instance),
				.glfw = std::move(lib),
				.window = std::move(window),
				.applicationInfo = std::move(info),
				.device = std::move(device),
				.physicalDevice = std::move(physical_device),
				.queue = std::move(queue),
				.surface = std::move(surface),
				.pipelineLayout = std::move(pipeline_layout),
				.renderPass = std::move(render_pass),
				.graphicsPipeline = std::move(graphics_pipeline),
				.commandPool = std::move(command_pool),
				.swapChainObject = std::move(swap_chain_object),
				.frameObjects = std::move(frame_objects)
			};
		}
	};

	ji::unique<Application> Application::create(ApplicationInfo&& info) {
		return ji::make_unique<ApplicationWindows>(std::move(info));
	}
}
