// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "mojo/services/view_manager/root_node_manager.h"

#include "base/logging.h"
#include "mojo/public/cpp/application/application_connection.h"
#include "mojo/public/interfaces/application/service_provider.mojom.h"
#include "mojo/services/public/cpp/input_events/input_events_type_converters.h"
#include "mojo/services/view_manager/view_manager_service_impl.h"
#include "ui/aura/env.h"

namespace mojo {
namespace service {

RootNodeManager::ScopedChange::ScopedChange(
    ViewManagerServiceImpl* connection,
    RootNodeManager* root,
    bool is_delete_node)
    : root_(root),
      connection_id_(connection->id()),
      is_delete_node_(is_delete_node) {
  root_->PrepareForChange(this);
}

RootNodeManager::ScopedChange::~ScopedChange() {
  root_->FinishChange();
}

RootNodeManager::Context::Context() {
  // Pass in false as native viewport creates the PlatformEventSource.
  aura::Env::CreateInstance(false);
}

RootNodeManager::Context::~Context() {
  aura::Env::DeleteInstance();
}

RootNodeManager::RootNodeManager(
    ApplicationConnection* app_connection,
    RootViewManagerDelegate* view_manager_delegate,
    const Callback<void()>& native_viewport_closed_callback)
    : app_connection_(app_connection),
      next_connection_id_(1),
      root_view_manager_(app_connection,
                         this,
                         view_manager_delegate,
                         native_viewport_closed_callback),
      root_(new Node(this, RootNodeId())),
      current_change_(NULL) {
}

RootNodeManager::~RootNodeManager() {
  while (!connections_created_by_connect_.empty())
    delete *(connections_created_by_connect_.begin());
  // All the connections should have been destroyed.
  DCHECK(connection_map_.empty());
  root_.reset();
}

ConnectionSpecificId RootNodeManager::GetAndAdvanceNextConnectionId() {
  const ConnectionSpecificId id = next_connection_id_++;
  DCHECK_LT(id, next_connection_id_);
  return id;
}

void RootNodeManager::AddConnection(ViewManagerServiceImpl* connection) {
  DCHECK_EQ(0u, connection_map_.count(connection->id()));
  connection_map_[connection->id()] = connection;
}

void RootNodeManager::RemoveConnection(ViewManagerServiceImpl* connection) {
  connection_map_.erase(connection->id());
  connections_created_by_connect_.erase(connection);

  // Notify remaining connections so that they can cleanup.
  for (ConnectionMap::const_iterator i = connection_map_.begin();
       i != connection_map_.end(); ++i) {
    i->second->OnViewManagerServiceImplDestroyed(connection->id());
  }
}

void RootNodeManager::EmbedRoot(
    const std::string& url,
    InterfaceRequest<ServiceProvider> service_provider) {
  if (connection_map_.empty()) {
    EmbedImpl(kInvalidConnectionId, String::From(url), RootNodeId(),
              service_provider.Pass());
    return;
  }
  ViewManagerServiceImpl* connection = GetConnection(kWindowManagerConnection);
  connection->client()->Embed(url, service_provider.Pass());
}

void RootNodeManager::Embed(
    ConnectionSpecificId creator_id,
    const String& url,
    Id transport_node_id,
    InterfaceRequest<ServiceProvider> service_provider) {
  EmbedImpl(creator_id,
            url,
            NodeIdFromTransportId(transport_node_id),
            service_provider.Pass())->set_delete_on_connection_error();
}

ViewManagerServiceImpl* RootNodeManager::GetConnection(
    ConnectionSpecificId connection_id) {
  ConnectionMap::iterator i = connection_map_.find(connection_id);
  return i == connection_map_.end() ? NULL : i->second;
}

Node* RootNodeManager::GetNode(const NodeId& id) {
  if (id == root_->id())
    return root_.get();
  ConnectionMap::iterator i = connection_map_.find(id.connection_id);
  return i == connection_map_.end() ? NULL : i->second->GetNode(id);
}

void RootNodeManager::OnConnectionMessagedClient(ConnectionSpecificId id) {
  if (current_change_)
    current_change_->MarkConnectionAsMessaged(id);
}

bool RootNodeManager::DidConnectionMessageClient(
    ConnectionSpecificId id) const {
  return current_change_ && current_change_->DidMessageConnection(id);
}

ViewManagerServiceImpl* RootNodeManager::GetConnectionByCreator(
    ConnectionSpecificId creator_id,
    const std::string& url) const {
  for (ConnectionMap::const_iterator i = connection_map_.begin();
       i != connection_map_.end(); ++i) {
    if (i->second->creator_id() == creator_id && i->second->url() == url)
      return i->second;
  }
  return NULL;
}

const ViewManagerServiceImpl* RootNodeManager::GetConnectionWithRoot(
    const NodeId& id) const {
  for (ConnectionMap::const_iterator i = connection_map_.begin();
       i != connection_map_.end(); ++i) {
    if (i->second->HasRoot(id))
      return i->second;
  }
  return NULL;
}

void RootNodeManager::DispatchNodeInputEventToWindowManager(EventPtr event) {
  // Input events are forwarded to the WindowManager. The WindowManager
  // eventually calls back to us with DispatchOnViewInputEvent().
  ViewManagerServiceImpl* connection = GetConnection(kWindowManagerConnection);
  if (!connection)
    return;
  connection->client()->DispatchOnViewInputEvent(event.Pass());
}

void RootNodeManager::ProcessNodeBoundsChanged(const Node* node,
                                               const gfx::Rect& old_bounds,
                                               const gfx::Rect& new_bounds) {
  for (ConnectionMap::iterator i = connection_map_.begin();
       i != connection_map_.end(); ++i) {
    i->second->ProcessNodeBoundsChanged(node, old_bounds, new_bounds,
                                        IsChangeSource(i->first));
  }
}

void RootNodeManager::ProcessNodeHierarchyChanged(const Node* node,
                                                  const Node* new_parent,
                                                  const Node* old_parent) {
  for (ConnectionMap::iterator i = connection_map_.begin();
       i != connection_map_.end(); ++i) {
    i->second->ProcessNodeHierarchyChanged(
        node, new_parent, old_parent, IsChangeSource(i->first));
  }
}

void RootNodeManager::ProcessNodeReorder(const Node* node,
                                         const Node* relative_node,
                                         const OrderDirection direction) {
  for (ConnectionMap::iterator i = connection_map_.begin();
       i != connection_map_.end(); ++i) {
    i->second->ProcessNodeReorder(
        node, relative_node, direction, IsChangeSource(i->first));
  }
}

void RootNodeManager::ProcessNodeDeleted(const NodeId& node) {
  for (ConnectionMap::iterator i = connection_map_.begin();
       i != connection_map_.end(); ++i) {
    i->second->ProcessNodeDeleted(node, IsChangeSource(i->first));
  }
}

void RootNodeManager::PrepareForChange(ScopedChange* change) {
  // Should only ever have one change in flight.
  CHECK(!current_change_);
  current_change_ = change;
}

void RootNodeManager::FinishChange() {
  // PrepareForChange/FinishChange should be balanced.
  CHECK(current_change_);
  current_change_ = NULL;
}

ViewManagerServiceImpl* RootNodeManager::EmbedImpl(
    const ConnectionSpecificId creator_id,
    const String& url,
    const NodeId& root_id,
    InterfaceRequest<ServiceProvider> service_provider) {
  MessagePipe pipe;

  ServiceProvider* view_manager_service_provider =
      app_connection_->ConnectToApplication(url)->GetServiceProvider();
  view_manager_service_provider->ConnectToService(
      ViewManagerServiceImpl::Client::Name_,
      pipe.handle1.Pass());

  std::string creator_url;
  ConnectionMap::const_iterator it = connection_map_.find(creator_id);
  if (it != connection_map_.end())
    creator_url = it->second->url();

  ViewManagerServiceImpl* connection =
      new ViewManagerServiceImpl(this,
                                 creator_id,
                                 creator_url,
                                 url.To<std::string>(),
                                 root_id,
                                 service_provider.Pass());
  WeakBindToPipe(connection, pipe.handle0.Pass());
  connections_created_by_connect_.insert(connection);
  OnConnectionMessagedClient(connection->id());
  return connection;
}

void RootNodeManager::OnNodeDestroyed(const Node* node) {
  ProcessNodeDeleted(node->id());
}

void RootNodeManager::OnNodeHierarchyChanged(const Node* node,
                                             const Node* new_parent,
                                             const Node* old_parent) {
  if (!root_view_manager_.in_setup())
    ProcessNodeHierarchyChanged(node, new_parent, old_parent);
}

void RootNodeManager::OnNodeBoundsChanged(const Node* node,
                                          const gfx::Rect& old_bounds,
                                          const gfx::Rect& new_bounds) {
  ProcessNodeBoundsChanged(node, old_bounds, new_bounds);
}

}  // namespace service
}  // namespace mojo
