#include "pr.hpp"
#include <iostream>
#include <memory>
#include <algorithm>

#include <pxr/usd/usd/stage.h>
#include <pxr/usd/usd/primRange.h>
#include <pxr/usd/usd/variantSets.h>
#include <pxr/usd/usd/editContext.h>
#include <pxr/usd/usd/modelApi.h>
#include <pxr/usd/sdf/types.h>
#include <pxr/usd/usdGeom/xform.h>
#include <pxr/usd/usdGeom/mesh.h>
#include <pxr/usd/usdGeom/sphere.h>
#include <pxr/usd/usdGeom/xformCommonAPI.h>
#include <pxr/usd/usdGeom/metrics.h>
#include <pxr/usd/usdGeom/tokens.h>
#include <pxr/usd/usdShade/material.h>
#include <pxr/usd/usdShade/materialBindingAPI.h>
#include <pxr/base/gf/matrix4f.h>

int main() {
    using namespace pr;


    Config config;
    config.ScreenWidth = 960;
    config.ScreenHeight = 960;
    config.SwapInterval = 1;
    Initialize(config);

    Camera3D camera;
    camera.origin = { 0, 0, 4 };
    camera.lookat = { 0, 0, 0 };
    camera.zUp = false;

    pr::SetDataDir(pr::ExecutableDir());
    auto pStage = pxr::UsdStage::Open(pr::GetDataPath("data/scene.usda"));

    bool drawwire = true;
    float time = 0.0f;

    SetDepthTest(true);

    double e = GetElapsedTime();

    while (pr::NextFrame() == false) {
        if (IsImGuiUsingMouse() == false) {
            UpdateCameraBlenderLike(&camera);
        }

        BeginCamera(camera);

        ClearBackground(0, 0, 0, 1);

        PushGraphicState();

        // DrawGrid(GridAxis::XZ, 1.0f, 10, { 128, 128, 128 });
        DrawXYZAxis(1.0f);

        if(drawwire)
        for (pxr::UsdPrim p : pStage->Traverse())
        {
            if (p.IsA<pxr::UsdGeomMesh>())
            {
                pxr::UsdGeomMesh mesh(p);
                pxr::VtArray<pxr::GfVec3f> points;
                pxr::VtArray<int> vertexCounts;
                pxr::VtArray<int> indices;
                mesh.GetPointsAttr().Get(&points);
                mesh.GetFaceVertexCountsAttr().Get(&vertexCounts);
                mesh.GetFaceVertexIndicesAttr().Get(&indices);

                pxr::GfMatrix4f matrix = pxr::GfMatrix4f(mesh.ComputeLocalToWorldTransform(pxr::UsdTimeCode(time)));
                glm::mat4 m;
                memcpy(glm::value_ptr(m), matrix.data(), sizeof(glm::mat4));
                pr::SetObjectTransform(m);

                for (int i = 0; i < points.size(); ++i)
                {
                    pxr::GfVec3f point = points[i];
                    DrawPoint({ point[0], point[1], point[2] }, { 255, 0, 0 }, 2);
                }

                pr::PrimBegin(pr::PrimitiveMode::Lines, 1);
                int indexStart = 0;
                for (int i = 0; i < vertexCounts.size(); ++i)
                {
                    int vn = vertexCounts[i];
                    int nLoopV = std::min(vn, 4);
                    for (int i = 0; i < nLoopV; ++i)
                    {
                        pxr::GfVec3f p0 = points[indices[indexStart + i]];
                        pxr::GfVec3f p1 = points[indices[indexStart + (i + 1) % nLoopV]];
                        pr::PrimVertex({ p0[0], p0[1], p0[2] }, { 255, 255, 255 });
                        pr::PrimVertex({ p1[0], p1[1], p1[2] }, { 255, 255, 255 });
                    }
                    indexStart += vn;
                }
                pr::PrimEnd();

                pr::SetObjectIdentify();
            }
        }

        PopGraphicState();
        EndCamera();

        BeginImGui();

        ImGui::SetNextWindowSize({ 500, 800 }, ImGuiCond_Once);
        ImGui::Begin("Panel");
        ImGui::Text("fps = %f", GetFrameRate());
        ImGui::Checkbox("drawwire", &drawwire);
        ImGui::SliderFloat("time", &time, 0, 24);

        for (pxr::UsdPrim p : pStage->Traverse())
        {
            if (p.IsA<pxr::UsdGeomMesh>())
            {
                pxr::UsdProperty prop = p.GetProperty(pxr::TfToken("MyMaterial"));
                pxr::UsdAttribute att = p.GetAttribute(prop.GetName());
                std::string value;
                att.Get(&value, pxr::UsdTimeCode(time));
                ImGui::Text("%s - %s : %s\n", p.GetPath().GetText(), prop.GetName().GetText(), value.c_str());
            }
        }

        ImGui::End();

        EndImGui();
    }

    pr::CleanUp();
}
