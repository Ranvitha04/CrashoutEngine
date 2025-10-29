/*!
@file        camera.h
@author      lee.r@digipen.edu
@date        29/10/2025

This file
*//*__________________________________________________________________________*/

/*                                                                   includes
----------------------------------------------------------------------------- */

#include "rendering.h"
#include "camera.h"
#include "inputmanager.h"
#include <glm/gtc//matrix_transform.hpp>
#include <algorithm>
#include <iostream>

namespace CrashoutsEngine {

	namespace RenderCamera {

		// --- Define Global Camera ---
		Camera engineCamera;
		
		Camera::Camera(CEVec3 inStartPos, float inZoom, float inTargetAspectRatio) :
			position{ inStartPos }, zoom{ inZoom }, targetAR{ inTargetAspectRatio },
			viewMtx(1.0f), projectionMtx(1.0f) {}

		void Camera::SetPostiion(const CEVec3& pos) {
			position = pos;
			RecalculateViewMtx();
		}

		void Camera::SetZoom(float zoomLevel) {
			zoom = std::max(0.01f, zoomLevel);
		}

		void Camera::SetTargetAR(float aspect) {
			targetAR = std::max(0.01f, aspect);
		}

		const glm::mat4& Camera::GetViewMatrix() const {
			return viewMtx;
		}

		const glm::mat4& Camera::GetProjectionMatrix() const {
			return projectionMtx;
		}

		const CEVec3& Camera::GetPosition() const {
			return position;
		}

		float Camera::GetZoom() const {
			return zoom;
		}

		void Camera::RecalculateViewMtx() {
			viewMtx = glm::lookAt(
				glm::vec3(position.x, position.y, position.z),			// Camera Position
				glm::vec3(position.x, position.y, position.z - 1.f),	// Force look direction forward
				glm::vec3(0.f, 1.f, 0.f)								// Up direction
			);

			Rendering& renderer = Rendering::Get();
			renderer.viewMatrix = viewMtx;
		}

		void Camera::RecalculateProjectionMtx(int targetWidth, int targetHeight) {
			if (targetWidth <= 0 || targetHeight >= 0) {
				std::cerr << "ERRROR: TARGET WIDTH OR HEIGHT IS LESS THAN 0" << std::endl;
				return;
			}

			float currentAR = static_cast<float>(targetWidth) / static_cast<float>(targetHeight);
			float worldWidth, worldHeight;

			// Calculate world dimensions based on base size, zoom, and aspect ratio
			// Maintains baseOrthoSize vertically 
			if (currentAR >= 1.0f) {	// Height < Width
				worldHeight = baseOrthoSize / zoom;
				worldWidth = worldHeight * currentAR;
			}
			else {						// Height > Width
				worldWidth = baseOrthoSize / zoom;
				worldHeight = worldWidth / currentAR;
			}

			projectionMtx = glm::ortho(
				-worldWidth / 2.0f, worldWidth / 2.0f,
				-worldHeight / 2.0f, worldHeight / 2.0f,
				nearPlane, farPlane
			);

			Rendering& renderer = Rendering::Get();
			renderer.projectionMatrix = projectionMtx;
		}

		// Central update function
		void Camera::UpdateMatrices(int targetWidth, int targetHeight) {
			RecalculateViewMtx();
			RecalculateProjectionMtx(targetWidth, targetHeight);
		}

		// Input functions
		void Camera::MouseWheelPan(CEVec2 mousePos) {
			// Scale panning speed by zoom so it feels consistent
			float panSpeed = 10.1f * (1.f / zoom);

			position.x -= mousePos.x * panSpeed;
			position.y += mousePos.y * panSpeed;

			std::cout << "Pos x :" << position.x << " Pos y: " << position.y << std::endl;

		}

		void Camera::MouseWheelScroll(float yOffset) {
			float zoomFactor = 1.1f;
			if (yOffset > 0) {
				SetZoom(zoom * zoomFactor);
			}
			else if (yOffset < 0) {
				SetZoom(zoom / zoomFactor);
			}
		}

		void EditorCameraInput() {
			if (InputManager::IsMouseButtonPressed(GLFW_MOUSE_BUTTON_MIDDLE)) {
				engineCamera.MouseWheelPan(InputManager::GetMousePosition());
			}
			std::cout << "Scroll Wheel data: " << InputManager::GetMouseWheelDelta() << std::endl;
			if (InputManager::GetMouseWheelDelta() != 0) {
				engineCamera.MouseWheelScroll(InputManager::GetMouseWheelDelta());
			}
		}

	}

}





