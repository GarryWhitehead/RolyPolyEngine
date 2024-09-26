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

#include "render_graph.h"

#include "render_graph_pass.h"
#include "render_pass_node.h"
#include "rendergraph_resource.h"
#include "resource_node.h"
#include "utility/assertion.h"
#include "vulkan-api/driver.h"
#include "vulkan-api/image.h"
#include "vulkan-api/renderpass.h"

#include <algorithm>


namespace yave::rg
{

RenderGraph::RenderGraph(vkapi::VkDriver& driver)
    : driver_(driver), blackboard_(std::make_unique<BlackBoard>())
{
}

RenderGraph::~RenderGraph() = default;

void RenderGraph::createPassNode(const util::CString& name, RenderGraphPassBase* rgPass)
{
    auto node = std::make_unique<RenderPassNode>(*this, rgPass, name);
    ASSERT_LOG(node);
    rgPass->setNode(node.get());
    rPassNodes_.emplace_back(std::move(node));
}

void RenderGraph::addPresentPass(const RenderGraphHandle& input)
{
    auto node = std::make_unique<PresentPassNode>(*this);
    ASSERT_LOG(node);
    RenderGraphBuilder builder {this, node.get()};
    addRead(input, node.get(), {});
    builder.addSideEffect();
    rPassNodes_.emplace_back(std::move(node));
}

RenderGraphHandle RenderGraph::addResource(std::unique_ptr<ResourceBase> resource)
{
    return addSubResource(std::move(resource), {});
}

RenderGraphHandle
RenderGraph::addSubResource(std::unique_ptr<ResourceBase> resource, const RenderGraphHandle& parent)
{
    RenderGraphHandle handle(static_cast<uint32_t>(resourceSlots_.size()));
    resourceSlots_.push_back({resources_.size(), resourceNodes_.size()});
    auto node = std::make_unique<ResourceNode>(*this, resource->getName(), handle, parent);
    resources_.emplace_back(std::move(resource));
    resourceNodes_.emplace_back(std::move(node));
    return handle;
}

RenderGraphHandle
RenderGraph::moveResource(const RenderGraphHandle& from, const RenderGraphHandle& to)
{
    ASSERT_LOG(from);
    ASSERT_LOG(to);

    auto& fromSlot = resourceSlots_[from.getKey()];
    auto& toSlot = resourceSlots_[to.getKey()];

    ResourceNode* fromNode = getResourceNode(from);
    ResourceNode* toNode = getResourceNode(to);

    // connect the replacement node to the forwarded node
    fromNode->setAliasResourceEdge(toNode);
    fromSlot.resourceIdx = toSlot.resourceIdx;

    return from;
}

ResourceNode* RenderGraph::getResourceNode(const RenderGraphHandle& handle)
{
    ASSERT_LOG(handle.getKey() < resourceSlots_.size());
    ResourceSlot& slot = resourceSlots_[handle.getKey()];
    return resourceNodes_[slot.nodeIdx].get();
}

RenderGraphHandle RenderGraph::importRenderTarget(
    const util::CString& name,
    const ImportedRenderTarget::Descriptor& importedDesc,
    const vkapi::RenderTargetHandle& handle)
{
    TextureResource::Descriptor resDesc {importedDesc.width, importedDesc.height};
    auto importedRT = std::make_unique<ImportedRenderTarget>(name, handle, resDesc, importedDesc);
    return addResource(std::move(importedRT));
}

RenderGraphHandle RenderGraph::addRead(
    const RenderGraphHandle& handle, PassNodeBase* passNode, vk::ImageUsageFlags usage)
{
    ASSERT_LOG(handle.getKey() < resources_.size());
    ResourceBase* resource = resources_[handle.getKey()].get();

    ASSERT_LOG(handle.getKey() < resourceNodes_.size());
    ResourceNode* node = resourceNodes_[handle.getKey()].get();

    ResourceBase::connectReader(dGraph_, passNode, node, usage);
    if (resource->isSubResource())
    {
        // if this is a subresource, it has a write dependency
        ResourceNode* parentNode = node->getParentNode();
        node->setParentWriter(parentNode);
    }

    return handle;
}

RenderGraphHandle RenderGraph::addWrite(
    const RenderGraphHandle& handle, PassNodeBase* passNode, vk::ImageUsageFlags usage)
{
    ASSERT_LOG(handle.getKey() < resources_.size());
    ResourceBase* resource = resources_[handle.getKey()].get();

    ASSERT_LOG(handle.getKey() < resourceNodes_.size());
    ResourceNode* node = resourceNodes_[handle.getKey()].get();

    ResourceBase::connectWriter(dGraph_, passNode, node, usage);

    // if its an imported resource, make sure its not culled
    if (resource->isImported())
    {
        passNode->declareSideEffect();
    }

    if (resource->isSubResource())
    {
        // if this is a subresource, it has a write dependency
        ResourceNode* parentNode = node->getParentNode();
        node->setParentWriter(parentNode);
    }

    return handle;
}

void RenderGraph::reset()
{
    dGraph_.clear();
    blackboard_->reset();
    rGraphPasses_.clear();
    resources_.clear();
    rPassNodes_.clear();
    resourceNodes_.clear();
    resourceSlots_.clear();
}

RenderGraph& RenderGraph::compile()
{
    dGraph_.cull();

    // partition the container so active nodes are at the
    // front and culled nodes are at the back
    activeNodesEnd_ = std::stable_partition(
        rPassNodes_.begin(), rPassNodes_.end(), [](std::unique_ptr<PassNodeBase>& node) {
            return !node->isCulled();
        });

    ASSERT_LOG(!rPassNodes_.empty());
    size_t nodeIdx = 0;
    auto lastNode = activeNodesEnd_;

    while (nodeIdx < rPassNodes_.size() && rPassNodes_[nodeIdx].get() != lastNode->get())
    {
        ASSERT_LOG(nodeIdx < rPassNodes_.size());
        PassNodeBase* passNode = rPassNodes_[nodeIdx++].get();

        const auto& readers = dGraph_.getReaderEdges(passNode);
        for (const auto* edge : readers)
        {
            auto* node = static_cast<ResourceNode*>(dGraph_.getNode(edge->fromId));
            passNode->addResource(node->resourceHandle());
        }

        const auto& writers = dGraph_.getWriterEdges(passNode);
        for (const auto* edge : writers)
        {
            auto* node = static_cast<ResourceNode*>(dGraph_.getNode(edge->toId));
            passNode->addResource(node->resourceHandle());
        }
        passNode->build();
    }

    // bake the resources
    for (auto& resource : resources_)
    {
        if (resource->readCount())
        {
            PassNodeBase* firstNode = resource->getFirstPassNode();
            PassNodeBase* lastNode = resource->getLastPassNode();

            if (firstNode && lastNode)
            {
                firstNode->addToBakeList(resource.get());
                lastNode->addToDestroyList(resource.get());
            }
        }
    }

    // update the usage flags for all resources
    for (size_t i = 0; i < resources_.size(); ++i)
    {
        ResourceNode* node = resourceNodes_[i].get();
        node->updateResourceUsage();
    }

    return *this;
}

void RenderGraph::execute()
{
    size_t nodeIdx = 0;
    auto lastNode = activeNodesEnd_;

    while (nodeIdx < rPassNodes_.size() && rPassNodes_[nodeIdx].get() != lastNode->get())
    {
        ASSERT_LOG(nodeIdx < rPassNodes_.size());
        auto* passNode = static_cast<RenderPassNode*>(rPassNodes_[nodeIdx++].get());

        // create concrete vulkan resources - these are added to the
        // node during the compile call
        passNode->bakeResourceList(driver_);

        RenderGraphResource resources(*this, passNode);
        passNode->execute(driver_, resources);

        // Resources used by the render graph are added to the garbage
        // collector to delay their destruction for a few frames so
        // we can be certain that the cmd buffers in flight have finished
        // with them.
        for (const auto& n : rPassNodes_)
        {
            n->destroyResourceList(driver_);
        }
    }
}

std::vector<std::unique_ptr<ResourceBase>>& RenderGraph::getResources() { return resources_; }

ResourceBase* RenderGraph::getResource(const RenderGraphHandle& handle) const
{
    ResourceSlot slot = resourceSlots_[handle.getKey()];
    ASSERT_FATAL(
        slot.resourceIdx < resources_.size(),
        "Resource index (=%d) is out of limits.",
        slot.resourceIdx);
    return resources_[slot.resourceIdx].get();
}

} // namespace yave::rg
