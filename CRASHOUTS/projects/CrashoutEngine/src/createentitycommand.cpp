#include "createentitycommand.h"
#include "sceneasset.h"

#include <string>
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

namespace CrashoutsEngine {

    CreateEntityCommand::CreateEntityCommand() {}

    void CreateEntityCommand::Execute()
    {
        ECS& ecs = *ECS::getInstance();

       
        if (m_entityJson.empty())
        {
            m_createdEntity = ecs.CreateEntity();

            rapidjson::Document doc;
            doc.SetArray();
            auto& allocator = doc.GetAllocator();

            rapidjson::Value entVal = Serialization::SerializeEntity(m_createdEntity, allocator);
            doc.PushBack(entVal, allocator);

            rapidjson::StringBuffer buffer;
            rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
            doc.Accept(writer);

            m_entityJson = buffer.GetString();
            return;
        }

    
        rapidjson::Document doc;
        doc.Parse(m_entityJson.c_str());

        if (doc.HasParseError() || !doc.IsArray() || doc.Empty())
            return;

        m_createdEntity = static_cast<Entity>(Serialization::DeserializeEntity(doc[0], nullptr));
    }

    void CreateEntityCommand::Undo()
    {
        if (m_createdEntity == 0)
            return;

        ECS& ecs = *ECS::getInstance();
        ecs.DestroyEntity(m_createdEntity);
        m_createdEntity = 0;
    }

} 