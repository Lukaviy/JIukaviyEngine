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

import Application;
import Pointers;

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

	struct FamilyQueueIndices {
		std::optional<std::uint32_t> graphicsFamily;
		std::optional<uint32_t> presentFamily;

		bool isComplete() const {
			return graphicsFamily && presentFamily;
		}
	};

	FamilyQueueIndices findQueueFamilies(const vk::raii::PhysicalDevice& device, const vk::raii::SurfaceKHR& surface) {
		FamilyQueueIndices result;

		const auto properties = device.getQueueFamilyProperties();

		for (auto i = 0u; i < properties.size(); ++i) {
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
				spdlog::info("[{}] {}", p_callback_data->pMessageIdName, p_callback_data->pMessage);
				return VK_FALSE;
			}
		};
	}

	bool checkValidationSupport() {
		const auto available_layers = vk::enumerateInstanceLayerProperties();

		spdlog::info("Available validation layers: [{}]\n", fmt::join(available_layers | std::ranges::views::transform([](const vk::LayerProperties& layer) {
			return std::string_view(layer.layerName.data(), layer.layerName.size());
		}), ", "));

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
			.apiVersion = VK_API_VERSION_1_0
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

		vk::PhysicalDeviceFeatures device_features{ .geometryShader = true };

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

	bool isDeviceSuitable(const vk::raii::PhysicalDevice& device, const vk::raii::SurfaceKHR& surface) {
		return
			device.getProperties().deviceType == vk::PhysicalDeviceType::eDiscreteGpu &&
			device.getFeatures().geometryShader &&
			findQueueFamilies(device, surface).isComplete() &&
			checkDeviceExtensionSupport(device)
		;
	}

	vk::raii::PhysicalDevice selectPhysicalDevice(const vk::raii::Instance& intance, const vk::raii::SurfaceKHR& surface) {
		const auto physical_devices = vk::raii::PhysicalDevices{ intance };

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
		const FamilyQueueIndices& indices
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
}

namespace ji {
	class ApplicationWindows : public Application {
	public:
		void run() override {
			while (!m_window.shouldClose()) {
				//window.swapBuffers();
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
			vk::raii::SurfaceKHR&& surface
		) :
			m_vkInstance(std::move(instance)),
			m_glfw(std::move(lib)),
			m_window(std::move(window)),
			m_applicationInfo(std::move(info)),
			m_vkDevice(std::move(device)),
			m_vkQueue(std::move(queue)),
			m_vkSurface(std::move(surface))
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
		std::optional<vk::raii::DebugUtilsMessengerEXT> m_debugMessenger;
	};

	unique<Application> Application::create(ApplicationInfo&& info) {
		auto lib = glfw::init();
		auto instance = vk::utils::createInstance(info);
		auto window = vk::utils::createWindow(info);
		auto surface = vk::utils::createSurface(instance, window);
		const auto physical_devices = vk::raii::PhysicalDevices{ instance };
		vk::utils::logPhysicalDevicesInfo(physical_devices);
		const auto physical_device = vk::utils::selectPhysicalDevice(physical_devices, surface);
		auto device = vk::utils::createDevice(physical_device, surface);
		auto queue = vk::utils::createQueue(device, vk::utils::findQueueFamilies(physical_device, surface));

		return make_unique<ApplicationWindows>(std::move(lib), std::move(info), std::move(instance), std::move(device), std::move(queue), std::move(window), std::move(surface));
	}
}
