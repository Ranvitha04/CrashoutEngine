/*!
@file   physicsutils.h
@author Renfred Lee
@date   02/11/2025

@brief  Declaration for types of forces and a force manager

All content (c) 2025 Digipen Institute of Technology Singapore. All rights reserved.

//__________________________________________________________________________*/

#pragma once

#include "CETypes.h"
#include "CEVec3.h"
#include <vector>
#include <numeric>
#include <algorithm>

namespace Physics {

	/*FLAGS*/
	constexpr CE::u32 SIMULATE_DYNAMICS{ 1 };

	/*enum COMMON_FORCES {
		GRAVITY
	};*/

	/** @brief LinearForce */
	struct LinearForce {
		CE::s32 id{};
		CEVec3 unitDir{ CEVec3{} };
		f32 magnitude{};
		f32 lifetime{};
		f32 age{};
		bool isActive{ false };
	};

	constexpr f32 DEFAULT_GRAVITY_VALUE{ 70.f };

	/** @brief ForceManager */
	class ForceManager {
	public:
		using ForceList = std::vector<LinearForce>;
		std::vector<LinearForce> forceList;

		ForceManager() {
			/*forceList.push_back(LinearForce{
				GRAVITY,
				CEVec3{0.f, -1.f, 0.f},
				DEFAULT_GRAVITY_VALUE,
				0.f,
				0.f,
				true
				});*/
		}

		CEVec3 ResolveForces(f32 dt, f32 s = 1.f) {

			CEVec3 accumulatedForce{};

			std::for_each(forceList.begin(), forceList.end(), [this, dt, &accumulatedForce, s](LinearForce& force) {
				this->ValidateAge(force);
				if (force.isActive) {
					accumulatedForce += (force.magnitude * s * force.unitDir);
					this->AccumulateAge(force.id, dt);
				}
				});
			return accumulatedForce;
		}


		ForceList::iterator findForce(CE::s32 id) {
			return std::find_if(forceList.begin(), forceList.end(), [id](LinearForce const& val) {
				return val.id == id;
				});
		}

		void AddForce(LinearForce&& force) {
			ForceList::iterator found{ findForce(force.id) };

			if (found == forceList.end()) {
				forceList.push_back(std::move(force));
				return;
			}

			*found = std::move(force);

		}
		void RemoveForce(CE::s32 id) {
			ForceList::iterator found{ findForce(id) };
			forceList.erase(found);
		}

		void SetLifeTime(CE::s32 id, f32 lifetime) {
			std::vector<LinearForce>::iterator found{ findForce(id) };

			found->lifetime = lifetime;
		}

		void AccumulateAge(CE::s32 id, f32 dt) {
			std::vector<LinearForce>::iterator found{ findForce(id) };
			if (!found->lifetime) return;
			found->age += dt;
		}

		void AccumulateAge(LinearForce& force, f32 dt) {
			if (!force.lifetime) return;
			force.age += dt;
		}

		void ValidateAge(CE::s32 id) {
			std::vector<LinearForce>::iterator found{ findForce(id) };

			if (found->lifetime <= found->age && found->lifetime) {
				found->age = 0.f;
				found->isActive = false;
			}
		}

		void ValidateAge(LinearForce& force) {

			if (force.lifetime <= force.age && force.lifetime) {
				force.age = 0.f;
				force.isActive = false;
			}
		}
	};

}

