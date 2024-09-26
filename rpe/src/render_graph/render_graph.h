/* Copyright (c) 2022 Garry Whitehead
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#pragma once

#include "backboard.h"
#include "dependency_graph.h"
#include "render_graph_builder.h"
#include "render_graph_handle.h"
#include "render_graph_pass.h"
#include "utility/bitset_enum.h"
#include "utility/cstring.h"
#include "vulkan-api/driver.h"
#include "vulkan-api/renderpass.h"

#include <algorithm>
#include <cstdint>
#include <functional>
#include <memory>
#include <vector>

namespace yave::rg
{

// forward declerations
class PassNodeBase;
class PassNodeBase;

class RenderGraph
{
public:
    explicit RenderGraph(vkapi::VkDriver& driver);
    ~RenderGraph();

    // not copyable
    RenderGraph(const RenderGraph&) = delete;
    RenderGraph& operator=(const RenderGraph&) = delete;

    template <typename Data, typename SetupFunc, typename ExecuteFunc>
    RenderGraphPass<Data, ExecuteFunc>&
    addPass(const util::CString& name, SetupFunc setup, ExecuteFunc&& execute);

    // similar to addPass but only executes and is not culled
    template <typename ExecuteFunc>
    void addExecutorPass(const util::CString& name, ExecuteFunc&& execute);

    void addPresentPass(const RenderGraphHandle& input);
    void createPassNode(const util::CString& name, RenderGraphPassBase* rgPass);

    /**
     * @brief optimises the render graph if possible and fills in all the blanks
     * - i.e. references, flags, etc.
     */
    RenderGraph& compile();

    /**
     * The execution of the render pass. You must build the pass and call
     * **prepare** before this function
     */
    void execute();

    void reset();

    RenderGraphHandle
    addRead(const RenderGraphHandle& handle, PassNodeBase* passNode, vk::ImageUsageFlags usage);

    RenderGraphHandle
    addWrite(const RenderGraphHandle& handle, PassNodeBase* passNode, vk::ImageUsageFlags usage);

    RenderGraphHandle importRenderTarget(
        const util::CString& name,
        const ImportedRenderTarget::Descriptor& importedDesc,
        const vkapi::RenderTargetHandle& handle);

    ResourceNode* getResourceNode(const RenderGraphHandle& handle);

    RenderGraphHandle moveResource(const RenderGraphHandle& from, const RenderGraphHandle& to);

    std::vector<std::unique_ptr<ResourceBase>>& getResources();

    [[nodiscard]] ResourceBase* getResource(const RenderGraphHandle& handle) const;

    RenderGraphHandle addResource(std::unique_ptr<ResourceBase> resource);

    RenderGraphHandle
    addSubResource(std::unique_ptr<ResourceBase> resource, const RenderGraphHandle& parent);

    BlackBoard* getBlackboard() { return blackboard_.get(); }

    // =================== getters ==================================

    DependencyGraph& getDependencyGraph() { return dGraph_; }
    [[nodiscard]] vkapi::VkDriver& driver() const { return driver_; }

private:
    DependencyGraph dGraph_;

    vkapi::VkDriver& driver_;

    /// a list of all the render passes
    std::vector<std::unique_ptr<RenderGraphPassBase>> rGraphPasses_;

    /// a virtual list of all the resources associated with this graph
    std::vector<std::unique_ptr<ResourceBase>> resources_;

    std::vector<std::unique_ptr<PassNodeBase>> rPassNodes_;
    std::vector<std::unique_ptr<ResourceNode>> resourceNodes_;

    std::vector<std::unique_ptr<PassNodeBase>>::iterator activeNodesEnd_;

    struct ResourceSlot
    {
        size_t resourceIdx = 0;
        size_t nodeIdx = 0;
    };
    std::vector<ResourceSlot> resourceSlots_;

    std::unique_ptr<BlackBoard> blackboard_;
};

template <typename Data, typename SetupFunc, typename ExecuteFunc>
RenderGraphPass<Data, ExecuteFunc>&
RenderGraph::addPass(const util::CString& name, SetupFunc setup, ExecuteFunc&& execute)
{
    auto rPass =
        std::make_unique<RenderGraphPass<Data, ExecuteFunc>>(std::forward<ExecuteFunc>(execute));
    createPassNode(name, rPass.get());
    RenderGraphBuilder builder {this, reinterpret_cast<PassNodeBase*>(rPassNodes_.back().get())};
    setup(builder, const_cast<Data&>(rPass->getData()));
    auto& output = *rPass.get();
    rGraphPasses_.emplace_back(std::move(rPass));
    return output;
}

template <typename ExecuteFunc>
void RenderGraph::addExecutorPass(const util::CString& name, ExecuteFunc&& execute)
{
    struct Empty
    {
    };

    addPass<Empty>(
        name,
        [](RenderGraphBuilder& builder, Empty&) { builder.addSideEffect(); },
        [execute](
            vkapi::VkDriver& driver, const Empty& data, const rg::RenderGraphResource& resources) {
            execute(driver);
        });
}

} // namespace yave::rg
