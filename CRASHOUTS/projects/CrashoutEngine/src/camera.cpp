/*!
@file        camera.h
@author      lee.r@digipen.edu
@date        29/10/2025

This file
*//*__________________________________________________________________________*/

/*                                                                   includes
----------------------------------------------------------------------------- */

#include "rendering.h"
#include "frameratecontroller.h"
#include "camera.h"
#include "inputmanager.h"
#include "entitymanager.h"
#include "transformcomponent.h"
#include <glm/gtc//matrix_transform.hpp>
#include <algorithm>
#include <iostream>
#include <glapp.h>

namespace CrashoutsEngine {

	namespace RenderCamera {

		// --- Define Static Camera Data ---
		glm::mat4 Camera::viewMtx = glm::mat4(1.f);
		glm::mat4 Camera::projectionMtx = glm::mat4(1.f);

		// Orthographic projection settings
		float Camera::baseOrthoSize = 100.f;
		float Camera::nearPlane = -100.f;
		float Camera::farPlane = 100.f;
		
		float Camera::g_zoom = 1.f;

		// --- Define Global Camera ---
		Camera engineCamera;
		Camera gameCamera;
		
		Camera::Camera(CEVec3 inStartPos, float inZoom, float inTargetAspectRatio) :
			position{ inStartPos }, zoom{ inZoom }, targetAR{ inTargetAspectRatio }{
			g_zoom = zoom;
		}

		void Camera::SetPostiion(const CEVec3& pos) {
			position = pos;
			RecalculateViewMtx();
		}

		void Camera::SetZoom(float zoomLevel) {
			zoom = std::max(0.01f, zoomLevel);
			g_zoom = zoom;
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
		float Camera::GetFar() const {
			return farPlane;
		}
		float Camera::GetNear() const {
			return nearPlane;
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
			if (targetWidth <= 0 || targetHeight <= 0) {
				std::cerr << "ERRROR: TARGET WIDTH OR HEIGHT IS LESS THAN 0" << std::endl;
				return;
			}

			// Calculate orthographic projection based on target dimensions

			float currentAR = static_cast<float>(targetWidth) / static_cast<float>(targetHeight);
			float worldWidth{ (float)GLApp::width }, worldHeight{ (float)GLApp::height };

#if USE_IMGUI_EDITOR
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
#endif

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
		void Camera::MouseWheelDrag(CEVec2& oldMousePos) {
			// Scale panning speed by zoom so it feels consistent
			float panSpeed = 0.1f * (1.f / zoom);
			CEVec2 mousePos = InputManager::GetMousePosition();
			float xDiff = mousePos.x - oldMousePos.x;
			float yDiff = mousePos.y - oldMousePos.y;

			std::cout << "X diff is " << xDiff << " and yDiff is " << yDiff << std::endl;
			
			position.x -= xDiff * panSpeed;
			position.y += yDiff * panSpeed;
			oldMousePos = mousePos;
			//std::cout << "Pos x :" << position.x << " Pos y: " << position.y << std::endl;

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
			static CEVec2 lastMousePos = InputManager::GetMousePosition();
			static bool middleMousePressed = false;

			if(!middleMousePressed) lastMousePos = InputManager::GetMousePosition();

			if (InputManager::IsMouseButtonPressed(GLFW_MOUSE_BUTTON_MIDDLE) || 
				InputManager::IsMouseButtonPressed(GLFW_MOUSE_BUTTON_RIGHT)) {
				middleMousePressed = true;
				engineCamera.MouseWheelDrag(lastMousePos);
			}
			else middleMousePressed = false;

			//std::cout << "Scroll Wheel data: " << InputManager::GetMouseWheelDelta() << std::endl;
			if (InputManager::GetMouseWheelDelta() != 0) {
				engineCamera.MouseWheelScroll(InputManager::GetMouseWheelDelta());
			}
		}

		void PlayerCameraInput(CameraComponent& cameraComponent, Entity& entity) {
			ECS& ecs = *ECS::getInstance();

			TransformComponent& target = *ecs.GetComponent<TransformComponent>(entity);
			float x = target.position.x;
			float y = target.position.y;

			// Maintain stated boundaries
			if ((cameraComponent.xBounds.y - cameraComponent.xBounds.x) > 0.f) {
				if (x > cameraComponent.xBounds.y) x = cameraComponent.xBounds.y;
				if (x < cameraComponent.xBounds.x) x = cameraComponent.xBounds.x;
			}

			if ((cameraComponent.yBounds.y - cameraComponent.yBounds.x) != 0.f) {
				if (y > cameraComponent.yBounds.y) y = cameraComponent.yBounds.y;
				if (y < cameraComponent.yBounds.x) y = cameraComponent.yBounds.x;
			}

			cameraComponent.position.x = x;
			cameraComponent.position.y = y;

			gameCamera.SetPostiion(cameraComponent.position);

			if(gameCamera.GetZoom() != cameraComponent.zoom) 
				gameCamera.SetZoom(cameraComponent.zoom);
		}
	}

}





