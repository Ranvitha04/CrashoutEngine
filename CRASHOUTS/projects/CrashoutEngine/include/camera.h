#ifndef CAMERA_H
#define CAMERA_H

/*!
@file        camera.h
@author      lee.r@digipen.edu
@date        29/10/2025

This file 
*//*__________________________________________________________________________*/

/*                                                                   includes
----------------------------------------------------------------------------- */

#include "CEVec2.h"
#include "CEVec3.h"
#include <glm/glm.hpp>

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

			// --- Update ---
			// Call this after changing position, zoom, or aspect ratio
			void UpdateMatrices(int targetWidth, int targetHeight);

			// --- Input Handling ---
			void MouseWheelDrag(CEVec2& oldMousePos);
			void MouseWheelScroll(float yOffset);

		private:
			CEVec3 position;
			float zoom;
			float targetAR;

			glm::mat4 viewMtx;
			glm::mat4 projectionMtx;

			// Orthographic projection settings
			float baseOrthoSize = 100.f;
			float nearPlane = -50.f;
			float farPlane = 50.f;

			void RecalculateViewMtx();
			void RecalculateProjectionMtx(int targetWidth, int targetHeight);

		};

		// --- Global Camera Instances ---
		extern Camera engineCamera;

		void EditorCameraInput();
	}
}


#endif
