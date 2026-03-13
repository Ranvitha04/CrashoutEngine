/*!
@file       camera.h
@author     Renfred Lee
@author     Renfred (RTIS)
@author     Shahrin Idzuan
@date       30/10/2025
@brief      [Brief description of camera.h]

All content � 2025 DigiPen Institute of Technology Singapore. All rights reserved.
*/
#ifndef CAMERA_H
#define CAMERA_H

/*!
@file        camera.h
@author      lee.r@digipen.edu
@date        29/10/2025

All content � 2025 Digipen Institute of Technology Singapore. All rights reserved.

This file 
*//*__________________________________________________________________________*/

/*                                                                   includes
----------------------------------------------------------------------------- */

#include "CEVec2.h"
#include "CEVec3.h"
#include "cameracomponent.h"
#include <glm/gtc/matrix_transform.hpp>

namespace CrashoutsEngine {

	namespace RenderCamera {

		class Camera {
		public:
			// --- Constructors ---
			Camera(CEVec3 startPos = CEVec3{0.f, 0.f, 0.f}, 
				float zoom = 1.0f,
				float targetAspectRatio = 16.0f / 9.0f);

			// --- Setters ---
			void SetPostiion(const CEVec3& pos);
			void SetZoom(float zoomLevel);
			void SetTargetAR(float aspect);

			// --- Getters ---
			const glm::mat4& GetViewMatrix() const;
			const glm::mat4& GetProjectionMatrix() const;
			const CEVec3& GetPosition() const;
			float GetZoom() const;
			float GetFar() const;
			float GetNear() const;

			// --- Update ---
			// Call this after changing position, zoom, or aspect ratio
			void UpdateMatrices(int targetWidth, int targetHeight);

			// --- Input Handling ---
			void MouseWheelDrag(CEVec2& oldMousePos);
			void MouseWheelScroll(float yOffset);

			// Get visible width and height
			float GetVisibleWorldHeight() const;
			float GetVisibleWorldWidth() const;

			static float g_zoom;

		private:
			CEVec3 position;
			float zoom;
			float targetAR;
			static glm::mat4 viewMtx;
			static glm::mat4 projectionMtx;

			// Orthographic projection settings
			static float baseOrthoSize;
			static float nearPlane;
			static float farPlane;


			void RecalculateViewMtx();
			void RecalculateProjectionMtx(int targetWidth, int targetHeight);

		};

		// --- Global Camera Instances ---
		extern Camera engineCamera;
		extern Camera gameCamera;

		void EditorCameraInput();

		void PlayerCameraInput(Component::CameraComponent& cameraComponent, Entity& entity);
	}
}


#endif

