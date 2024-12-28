#pragma once

#include "USD.h"
#include "USDImportContext.h"

#include "USDIncludeBegin.h"
#include <pxr/usd/usd/prim.h>
#include "USDIncludeEnd.h"

namespace Horizon::USDImporter
{
    class USDPrimImporter
    {
    public:
        USDPrimImporter(const USDImportContext& context, const pxr::UsdPrim& prim);
        virtual ~USDPrimImporter();

        const std::string& GetName() const;

        const std::string& GetPrimPath() const;

        const pxr::UsdPrim& GetPrim() const
        {
            return prim;
        }

        USDPrimImporter* GetParent() const
        {
            return parent;
        }

        EntityHandle GetEntity() const
        {
            return entity;
        }

        void SetParent(USDPrimImporter* parent)
        {
            this->parent = parent;
        }

        virtual void CreateEntity(Scene* scene) = 0;
        virtual void AddComponents(Scene* scene) = 0;

    protected:
        const USDImportContext* context;
        pxr::UsdPrim prim;
        USDPrimImporter* parent;
        EntityHandle entity;
    };
}