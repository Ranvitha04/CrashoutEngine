/*!
@file      contactinfobindings.cpp
@author    m.biniswandi@digipen.edu
@date      30/11/2025
@brief     Mono bindings to expose ContactInfo data to C#

All content © 2025 Digipen Institute of Technology Singapore. All rights reserved.

*/

#include "contactinfobindings.h"
#include "collidereventmanager.h"
#include "entitymanager.h"
#include "monomanager.h"

#include <mono/jit/jit.h>
#include <mono/metadata/object.h>
#include <mono/metadata/assembly.h>
#include <mono/metadata/mono-config.h>

using namespace CrashoutsEngine;

namespace
{
    // Create CEVec3 managed object
    static MonoObject* CreateManagedCEVec3(const CEVec3& v)
    {
        MonoImage* image = MonoManager::GetImage();
        MonoClass* vecClass = mono_class_from_name(image, "CrashoutsEngine", "CEVec3");

        MonoObject* obj = mono_object_new(mono_domain_get(), vecClass);
        mono_runtime_object_init(obj);

        MonoClassField* fx = mono_class_get_field_from_name(vecClass, "x");
        MonoClassField* fy = mono_class_get_field_from_name(vecClass, "y");
        MonoClassField* fz = mono_class_get_field_from_name(vecClass, "z");

        mono_field_set_value(obj, fx, (void*)&v.x);
        mono_field_set_value(obj, fy, (void*)&v.y);
        mono_field_set_value(obj, fz, (void*)&v.z);

        return obj;
    }

    static bool TryGetContactInfoForEntity(Entity entity, ContactInfo& outInfo)
    {
        return CollisionEventManager::GetInstance().TryGetLastContact(entity, outInfo);
    }

    static void Internal_FillContactInfo(uint32_t entityID, MonoObject* managedInfo)
    {
        if (!managedInfo)
            return;

        ContactInfo native{};
        if (!TryGetContactInfoForEntity((Entity)entityID, native))
            return;

        //Get C# ContactInfo class
        MonoClass* infoClass = mono_object_get_class(managedInfo);

        MonoClassField* fCollidedEntity = mono_class_get_field_from_name(infoClass, "collidedEntity");

        MonoClassField* fManifold = mono_class_get_field_from_name(infoClass, "manifold");

        // Read ContactInfo.manifold object
        MonoObject* manifoldObj = nullptr;
        mono_field_get_value(managedInfo, fManifold, &manifoldObj);

        if (!manifoldObj)
            return;

        MonoClass* manifoldClass = mono_object_get_class(manifoldObj);

        // Manifold fields
        MonoClassField* fNormal = mono_class_get_field_from_name(manifoldClass, "normal");

        MonoClassField* fContactPoints = mono_class_get_field_from_name(manifoldClass, "contactPoints");

        MonoClassField* fDepth = mono_class_get_field_from_name(manifoldClass, "depth");

        MonoClassField* fOccurred = mono_class_get_field_from_name(manifoldClass, "collisionOccured");

        MonoClassField* fNumContacts = mono_class_get_field_from_name(manifoldClass, "numOfContacts");

        // Set collided entity
        uint32_t other = (uint32_t)native.collidedEntity;
        mono_field_set_value(managedInfo, fCollidedEntity, &other);

        // Native manifold
        const Collision::CollisionManifold& col = native.info;

        mono_field_set_value(manifoldObj, fDepth, (void*)&col.depth);
        mono_field_set_value(manifoldObj, fOccurred, (void*)&col.collisionOccured);
        mono_field_set_value(manifoldObj, fNumContacts, (void*)&col.numOfContacts);

        // NORMAL CEVec3
        {
            MonoObject* normalObj = CreateManagedCEVec3(col.normal);
            mono_field_set_value(manifoldObj, fNormal, normalObj);
        }

        // CONTACT POINTS CEVec3[2]
        MonoArray* cpArray = nullptr;
        mono_field_get_value(manifoldObj, fContactPoints, &cpArray);

        if (cpArray != nullptr)
        {
            for (int i = 0; i < 2; i++)
            {
                MonoObject* cp =
                    CreateManagedCEVec3(col.contactPoints[i]);
                mono_array_set(cpArray, MonoObject*, i, cp);
            }
        }
    }
} // namespace


namespace CrashoutsEngine::Bindings
{
    void RegisterContactInfoBindings()
    {
        mono_add_internal_call("CrashoutsEngine.ContactInfo::Internal_Fill",(const void*)Internal_FillContactInfo);
    }
}
