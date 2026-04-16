#pragma once

#include "Events/Event.h"
#include "Events/KeyEvent.h"
#include "Events/MouseEvent.h"
#include "ECS/SceneManager.h"
#include "ECS/Scene.h"

#include <glm/gtc/matrix_transform.hpp>

namespace Orion {

    class ORION_API Action 
    {
        public:
            Action() = default;
            ~Action() = default;

            virtual void Execute() = 0;
            virtual void Undo() = 0;
         
    };

    class ORION_API TransformAction : public Action 
    {
    public:
        TransformAction(EntityID id,
            glm::vec3 oldP, glm::vec3 newP,
            glm::vec3 oldR, glm::vec3 newR,
            glm::vec3 oldS, glm::vec3 newS)
            : m_EntityID(id),
            m_OldPos(oldP), m_NewPos(newP),
            m_OldRot(oldR), m_NewRot(newR),
            m_OldScale(oldS), m_NewScale(newS) {}

        void Undo() override
        {
            // revert to the "old" transform for the entity
            auto* tc = SceneManager::GetActiveScene()->GetTransformComponent(m_EntityID);
            if (tc)
            {
                tc->position = m_OldPos;
                tc->rotation = m_OldRot;
                tc->scale = m_OldScale;
            }
        }

        void Execute() override
        {
            // apply the "new" transform to the entity
            auto* tc = SceneManager::GetActiveScene()->GetTransformComponent(m_EntityID);
            if (tc)
            {
                tc->position = m_NewPos;
                tc->rotation = m_NewRot;
                tc->scale = m_NewScale;
            }
        }

    private:
        EntityID m_EntityID; // affected entity
        glm::vec3 m_OldPos, m_NewPos;
        glm::vec3 m_OldRot, m_NewRot;
        glm::vec3 m_OldScale, m_NewScale;
    };

    class ORION_API CreateAction : public Action
    {
        void Undo() override
        {

        }

        void Execute() override
        {

        }
    };

    class ORION_API DestroyAction : public Action
    {
        void Undo() override
        {

        }

        void Execute() override
        {

        }
    };

    class ORION_API ParentAction : public Action
    {
        void Undo() override
        {

        }

        void Execute() override
        {

        }

        private:
            EntityID parentID;
            EntityID childID;
    }; 
}