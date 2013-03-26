// Copyright 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/trees/tree_synchronizer.h"

#include <algorithm>
#include <vector>

#include "cc/animation/layer_animation_controller.h"
#include "cc/layers/layer.h"
#include "cc/layers/layer_impl.h"
#include "cc/test/animation_test_common.h"
#include "cc/test/fake_impl_proxy.h"
#include "cc/test/fake_layer_tree_host_impl.h"
#include "cc/trees/proxy.h"
#include "cc/trees/single_thread_proxy.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace cc {
namespace {

class MockLayerImpl : public LayerImpl {
 public:
  static scoped_ptr<MockLayerImpl> Create(LayerTreeImpl* tree_impl,
                                          int layer_id) {
    return make_scoped_ptr(new MockLayerImpl(tree_impl, layer_id));
  }
  virtual ~MockLayerImpl() {
    if (layer_impl_destruction_list_)
      layer_impl_destruction_list_->push_back(id());
  }

  void SetLayerImplDestructionList(std::vector<int>* list) {
    layer_impl_destruction_list_ = list;
  }

 private:
  MockLayerImpl(LayerTreeImpl* tree_impl, int layer_id)
      : LayerImpl(tree_impl, layer_id),
        layer_impl_destruction_list_(NULL) {}

  std::vector<int>* layer_impl_destruction_list_;
};

class MockLayer : public Layer {
 public:
  static scoped_refptr<MockLayer> Create(
      std::vector<int>* layer_impl_destruction_list) {
    return make_scoped_refptr(new MockLayer(layer_impl_destruction_list));
  }

  virtual scoped_ptr<LayerImpl> CreateLayerImpl(LayerTreeImpl* tree_impl)
      OVERRIDE {
    return MockLayerImpl::Create(tree_impl, layer_id_).PassAs<LayerImpl>();
  }

  virtual void PushPropertiesTo(LayerImpl* layer_impl) OVERRIDE {
    Layer::PushPropertiesTo(layer_impl);

    MockLayerImpl* mock_layer_impl = static_cast<MockLayerImpl*>(layer_impl);
    mock_layer_impl->SetLayerImplDestructionList(layer_impl_destruction_list_);
  }

 private:
  explicit MockLayer(std::vector<int>* layer_impl_destruction_list)
      : Layer(), layer_impl_destruction_list_(layer_impl_destruction_list) {}
  virtual ~MockLayer() {}

  std::vector<int>* layer_impl_destruction_list_;
};

class FakeLayerAnimationController : public LayerAnimationController {
 public:
  static scoped_refptr<LayerAnimationController> Create() {
    return static_cast<LayerAnimationController*>(
        new FakeLayerAnimationController);
  }

  bool SynchronizedAnimations() const { return synchronized_animations_; }

 private:
  FakeLayerAnimationController()
      : LayerAnimationController(1),
        synchronized_animations_(false) {}

  virtual ~FakeLayerAnimationController() {}

  virtual void PushAnimationUpdatesTo(LayerAnimationController* controller_impl)
      OVERRIDE {
    LayerAnimationController::PushAnimationUpdatesTo(controller_impl);
    synchronized_animations_ = true;
  }

  bool synchronized_animations_;
};

void ExpectTreesAreIdentical(Layer* layer,
                             LayerImpl* layer_impl,
                             LayerTreeImpl* tree_impl) {
  ASSERT_TRUE(layer);
  ASSERT_TRUE(layer_impl);

  EXPECT_EQ(layer->id(), layer_impl->id());
  EXPECT_EQ(layer_impl->layer_tree_impl(), tree_impl);

  EXPECT_EQ(layer->non_fast_scrollable_region(),
            layer_impl->non_fast_scrollable_region());

  ASSERT_EQ(!!layer->mask_layer(), !!layer_impl->mask_layer());
  if (layer->mask_layer()) {
    ExpectTreesAreIdentical(
        layer->mask_layer(), layer_impl->mask_layer(), tree_impl);
  }

  ASSERT_EQ(!!layer->replica_layer(), !!layer_impl->replica_layer());
  if (layer->replica_layer()) {
    ExpectTreesAreIdentical(
        layer->replica_layer(), layer_impl->replica_layer(), tree_impl);
  }

  const std::vector<scoped_refptr<Layer> >& layer_children = layer->children();
  const ScopedPtrVector<LayerImpl>& layer_impl_children =
      layer_impl->children();

  ASSERT_EQ(layer_children.size(), layer_impl_children.size());

  for (size_t i = 0; i < layer_children.size(); ++i) {
    ExpectTreesAreIdentical(
        layer_children[i].get(), layer_impl_children[i], tree_impl);
  }
}

class TreeSynchronizerTest : public testing::Test {
 public:
  TreeSynchronizerTest() : host_impl_(&proxy_) {}

 protected:
  FakeImplProxy proxy_;
  FakeLayerTreeHostImpl host_impl_;
};

// Attempts to synchronizes a null tree. This should not crash, and should
// return a null tree.
TEST_F(TreeSynchronizerTest, SyncNullTree) {
  scoped_ptr<LayerImpl> layer_impl_tree_root =
      TreeSynchronizer::SynchronizeTrees(static_cast<Layer*>(NULL),
                                         scoped_ptr<LayerImpl>(),
                                         host_impl_.active_tree());

  EXPECT_TRUE(!layer_impl_tree_root.get());
}

// Constructs a very simple tree and synchronizes it without trying to reuse any
// preexisting layers.
TEST_F(TreeSynchronizerTest, SyncSimpleTreeFromEmpty) {
  scoped_refptr<Layer> layer_tree_root = Layer::Create();
  layer_tree_root->AddChild(Layer::Create());
  layer_tree_root->AddChild(Layer::Create());

  scoped_ptr<LayerImpl> layer_impl_tree_root =
      TreeSynchronizer::SynchronizeTrees(layer_tree_root.get(),
                                         scoped_ptr<LayerImpl>(),
                                         host_impl_.active_tree());

  ExpectTreesAreIdentical(layer_tree_root.get(),
                          layer_impl_tree_root.get(),
                          host_impl_.active_tree());
}

// Constructs a very simple tree and synchronizes it attempting to reuse some
// layers
TEST_F(TreeSynchronizerTest, SyncSimpleTreeReusingLayers) {
  std::vector<int> layer_impl_destruction_list;

  scoped_refptr<Layer> layer_tree_root =
      MockLayer::Create(&layer_impl_destruction_list);
  layer_tree_root->AddChild(MockLayer::Create(&layer_impl_destruction_list));
  layer_tree_root->AddChild(MockLayer::Create(&layer_impl_destruction_list));

  scoped_ptr<LayerImpl> layer_impl_tree_root =
      TreeSynchronizer::SynchronizeTrees(layer_tree_root.get(),
                                         scoped_ptr<LayerImpl>(),
                                         host_impl_.active_tree());
  ExpectTreesAreIdentical(layer_tree_root.get(),
                          layer_impl_tree_root.get(),
                          host_impl_.active_tree());

  // We have to push properties to pick up the destruction list pointer.
  TreeSynchronizer::PushProperties(layer_tree_root.get(),
                                   layer_impl_tree_root.get());

  // Add a new layer to the Layer side
  layer_tree_root->children()[0]->
      AddChild(MockLayer::Create(&layer_impl_destruction_list));
  // Remove one.
  layer_tree_root->children()[1]->RemoveFromParent();
  int second_layer_impl_id = layer_impl_tree_root->children()[1]->id();

  // Synchronize again. After the sync the trees should be equivalent and we
  // should have created and destroyed one LayerImpl.
  layer_impl_tree_root =
      TreeSynchronizer::SynchronizeTrees(layer_tree_root.get(),
                                         layer_impl_tree_root.Pass(),
                                         host_impl_.active_tree());
  ExpectTreesAreIdentical(layer_tree_root.get(),
                          layer_impl_tree_root.get(),
                          host_impl_.active_tree());

  ASSERT_EQ(1u, layer_impl_destruction_list.size());
  EXPECT_EQ(second_layer_impl_id, layer_impl_destruction_list[0]);
}

// Constructs a very simple tree and checks that a stacking-order change is
// tracked properly.
TEST_F(TreeSynchronizerTest, SyncSimpleTreeAndTrackStackingOrderChange) {
  std::vector<int> layer_impl_destruction_list;

  // Set up the tree and sync once. child2 needs to be synced here, too, even
  // though we remove it to set up the intended scenario.
  scoped_refptr<Layer> layer_tree_root =
      MockLayer::Create(&layer_impl_destruction_list);
  scoped_refptr<Layer> child2 = MockLayer::Create(&layer_impl_destruction_list);
  layer_tree_root->AddChild(MockLayer::Create(&layer_impl_destruction_list));
  layer_tree_root->AddChild(child2);
  scoped_ptr<LayerImpl> layer_impl_tree_root =
      TreeSynchronizer::SynchronizeTrees(layer_tree_root.get(),
                                         scoped_ptr<LayerImpl>(),
                                         host_impl_.active_tree());
  ExpectTreesAreIdentical(layer_tree_root.get(),
                          layer_impl_tree_root.get(),
                          host_impl_.active_tree());

  // We have to push properties to pick up the destruction list pointer.
  TreeSynchronizer::PushProperties(layer_tree_root.get(),
                                   layer_impl_tree_root.get());

  layer_impl_tree_root->ResetAllChangeTrackingForSubtree();

  // re-insert the layer and sync again.
  child2->RemoveFromParent();
  layer_tree_root->AddChild(child2);
  layer_impl_tree_root =
      TreeSynchronizer::SynchronizeTrees(layer_tree_root.get(),
                                         layer_impl_tree_root.Pass(),
                                         host_impl_.active_tree());
  ExpectTreesAreIdentical(layer_tree_root.get(),
                          layer_impl_tree_root.get(),
                          host_impl_.active_tree());

  TreeSynchronizer::PushProperties(layer_tree_root.get(),
                                   layer_impl_tree_root.get());

  // Check that the impl thread properly tracked the change.
  EXPECT_FALSE(layer_impl_tree_root->LayerPropertyChanged());
  EXPECT_FALSE(layer_impl_tree_root->children()[0]->LayerPropertyChanged());
  EXPECT_TRUE(layer_impl_tree_root->children()[1]->LayerPropertyChanged());
}

TEST_F(TreeSynchronizerTest, SyncSimpleTreeAndProperties) {
  scoped_refptr<Layer> layer_tree_root = Layer::Create();
  layer_tree_root->AddChild(Layer::Create());
  layer_tree_root->AddChild(Layer::Create());

  // Pick some random properties to set. The values are not important, we're
  // just testing that at least some properties are making it through.
  gfx::PointF root_position = gfx::PointF(2.3f, 7.4f);
  layer_tree_root->SetPosition(root_position);

  float first_child_opacity = 0.25f;
  layer_tree_root->children()[0]->SetOpacity(first_child_opacity);

  gfx::Size second_child_bounds = gfx::Size(25, 53);
  layer_tree_root->children()[1]->SetBounds(second_child_bounds);

  scoped_ptr<LayerImpl> layer_impl_tree_root =
      TreeSynchronizer::SynchronizeTrees(layer_tree_root.get(),
                                         scoped_ptr<LayerImpl>(),
                                         host_impl_.active_tree());
  ExpectTreesAreIdentical(layer_tree_root.get(),
                          layer_impl_tree_root.get(),
                          host_impl_.active_tree());

  TreeSynchronizer::PushProperties(layer_tree_root.get(),
                                   layer_impl_tree_root.get());

  // Check that the property values we set on the Layer tree are reflected in
  // the LayerImpl tree.
  gfx::PointF root_layer_impl_position = layer_impl_tree_root->position();
  EXPECT_EQ(root_position.x(), root_layer_impl_position.x());
  EXPECT_EQ(root_position.y(), root_layer_impl_position.y());

  EXPECT_EQ(first_child_opacity,
            layer_impl_tree_root->children()[0]->opacity());

  gfx::Size second_layer_impl_child_bounds =
      layer_impl_tree_root->children()[1]->bounds();
  EXPECT_EQ(second_child_bounds.width(),
            second_layer_impl_child_bounds.width());
  EXPECT_EQ(second_child_bounds.height(),
            second_layer_impl_child_bounds.height());
}

TEST_F(TreeSynchronizerTest, ReuseLayerImplsAfterStructuralChange) {
  std::vector<int> layer_impl_destruction_list;

  // Set up a tree with this sort of structure:
  // root --- A --- B ---+--- C
  //                     |
  //                     +--- D
  scoped_refptr<Layer> layer_tree_root =
      MockLayer::Create(&layer_impl_destruction_list);
  layer_tree_root->AddChild(MockLayer::Create(&layer_impl_destruction_list));

  scoped_refptr<Layer> layer_a = layer_tree_root->children()[0].get();
  layer_a->AddChild(MockLayer::Create(&layer_impl_destruction_list));

  scoped_refptr<Layer> layer_b = layer_a->children()[0].get();
  layer_b->AddChild(MockLayer::Create(&layer_impl_destruction_list));

  scoped_refptr<Layer> layer_c = layer_b->children()[0].get();
  layer_b->AddChild(MockLayer::Create(&layer_impl_destruction_list));
  scoped_refptr<Layer> layer_d = layer_b->children()[1].get();

  scoped_ptr<LayerImpl> layer_impl_tree_root =
      TreeSynchronizer::SynchronizeTrees(layer_tree_root.get(),
                                         scoped_ptr<LayerImpl>(),
                                         host_impl_.active_tree());
  ExpectTreesAreIdentical(layer_tree_root.get(),
                          layer_impl_tree_root.get(),
                          host_impl_.active_tree());

  // We have to push properties to pick up the destruction list pointer.
  TreeSynchronizer::PushProperties(layer_tree_root.get(),
                                   layer_impl_tree_root.get());

  // Now restructure the tree to look like this:
  // root --- D ---+--- A
  //               |
  //               +--- C --- B
  layer_tree_root->RemoveAllChildren();
  layer_d->RemoveAllChildren();
  layer_tree_root->AddChild(layer_d);
  layer_a->RemoveAllChildren();
  layer_d->AddChild(layer_a);
  layer_c->RemoveAllChildren();
  layer_d->AddChild(layer_c);
  layer_b->RemoveAllChildren();
  layer_c->AddChild(layer_b);

  // After another synchronize our trees should match and we should not have
  // destroyed any LayerImpls
  layer_impl_tree_root =
      TreeSynchronizer::SynchronizeTrees(layer_tree_root.get(),
                                         layer_impl_tree_root.Pass(),
                                         host_impl_.active_tree());
  ExpectTreesAreIdentical(layer_tree_root.get(),
                          layer_impl_tree_root.get(),
                          host_impl_.active_tree());

  EXPECT_EQ(0u, layer_impl_destruction_list.size());
}

// Constructs a very simple tree, synchronizes it, then synchronizes to a
// totally new tree. All layers from the old tree should be deleted.
TEST_F(TreeSynchronizerTest, SyncSimpleTreeThenDestroy) {
  std::vector<int> layer_impl_destruction_list;

  scoped_refptr<Layer> old_layer_tree_root =
      MockLayer::Create(&layer_impl_destruction_list);
  old_layer_tree_root->AddChild(
      MockLayer::Create(&layer_impl_destruction_list));
  old_layer_tree_root->AddChild(
      MockLayer::Create(&layer_impl_destruction_list));

  int old_tree_root_layer_id = old_layer_tree_root->id();
  int old_tree_first_child_layer_id = old_layer_tree_root->children()[0]->id();
  int old_tree_second_child_layer_id = old_layer_tree_root->children()[1]->id();

  scoped_ptr<LayerImpl> layer_impl_tree_root =
      TreeSynchronizer::SynchronizeTrees(old_layer_tree_root.get(),
                                         scoped_ptr<LayerImpl>(),
                                         host_impl_.active_tree());
  ExpectTreesAreIdentical(old_layer_tree_root.get(),
                          layer_impl_tree_root.get(),
                          host_impl_.active_tree());

  // We have to push properties to pick up the destruction list pointer.
  TreeSynchronizer::PushProperties(old_layer_tree_root.get(),
                                   layer_impl_tree_root.get());

  // Remove all children on the Layer side.
  old_layer_tree_root->RemoveAllChildren();

  // Synchronize again. After the sync all LayerImpls from the old tree should
  // be deleted.
  scoped_refptr<Layer> new_layer_tree_root = Layer::Create();
  layer_impl_tree_root =
      TreeSynchronizer::SynchronizeTrees(new_layer_tree_root.get(),
                                         layer_impl_tree_root.Pass(),
                                         host_impl_.active_tree());
  ExpectTreesAreIdentical(new_layer_tree_root.get(),
                          layer_impl_tree_root.get(),
                          host_impl_.active_tree());

  ASSERT_EQ(3u, layer_impl_destruction_list.size());

  EXPECT_TRUE(std::find(layer_impl_destruction_list.begin(),
                        layer_impl_destruction_list.end(),
                        old_tree_root_layer_id) !=
              layer_impl_destruction_list.end());
  EXPECT_TRUE(std::find(layer_impl_destruction_list.begin(),
                        layer_impl_destruction_list.end(),
                        old_tree_first_child_layer_id) !=
              layer_impl_destruction_list.end());
  EXPECT_TRUE(std::find(layer_impl_destruction_list.begin(),
                        layer_impl_destruction_list.end(),
                        old_tree_second_child_layer_id) !=
              layer_impl_destruction_list.end());
}

// Constructs+syncs a tree with mask, replica, and replica mask layers.
TEST_F(TreeSynchronizerTest, SyncMaskReplicaAndReplicaMaskLayers) {
  scoped_refptr<Layer> layer_tree_root = Layer::Create();
  layer_tree_root->AddChild(Layer::Create());
  layer_tree_root->AddChild(Layer::Create());
  layer_tree_root->AddChild(Layer::Create());

  // First child gets a mask layer.
  scoped_refptr<Layer> mask_layer = Layer::Create();
  layer_tree_root->children()[0]->SetMaskLayer(mask_layer.get());

  // Second child gets a replica layer.
  scoped_refptr<Layer> replica_layer = Layer::Create();
  layer_tree_root->children()[1]->SetReplicaLayer(replica_layer.get());

  // Third child gets a replica layer with a mask layer.
  scoped_refptr<Layer> replica_layer_with_mask = Layer::Create();
  scoped_refptr<Layer> replica_mask_layer = Layer::Create();
  replica_layer_with_mask->SetMaskLayer(replica_mask_layer.get());
  layer_tree_root->children()[2]->
      SetReplicaLayer(replica_layer_with_mask.get());

  scoped_ptr<LayerImpl> layer_impl_tree_root =
      TreeSynchronizer::SynchronizeTrees(layer_tree_root.get(),
                                         scoped_ptr<LayerImpl>(),
                                         host_impl_.active_tree());

  ExpectTreesAreIdentical(layer_tree_root.get(),
                          layer_impl_tree_root.get(),
                          host_impl_.active_tree());

  // Remove the mask layer.
  layer_tree_root->children()[0]->SetMaskLayer(NULL);
  layer_impl_tree_root =
      TreeSynchronizer::SynchronizeTrees(layer_tree_root.get(),
                                         layer_impl_tree_root.Pass(),
                                         host_impl_.active_tree());
  ExpectTreesAreIdentical(layer_tree_root.get(),
                          layer_impl_tree_root.get(),
                          host_impl_.active_tree());

  // Remove the replica layer.
  layer_tree_root->children()[1]->SetReplicaLayer(NULL);
  layer_impl_tree_root =
      TreeSynchronizer::SynchronizeTrees(layer_tree_root.get(),
                                         layer_impl_tree_root.Pass(),
                                         host_impl_.active_tree());
  ExpectTreesAreIdentical(layer_tree_root.get(),
                          layer_impl_tree_root.get(),
                          host_impl_.active_tree());

  // Remove the replica mask.
  replica_layer_with_mask->SetMaskLayer(NULL);
  layer_impl_tree_root =
      TreeSynchronizer::SynchronizeTrees(layer_tree_root.get(),
                                         layer_impl_tree_root.Pass(),
                                         host_impl_.active_tree());
  ExpectTreesAreIdentical(layer_tree_root.get(),
                          layer_impl_tree_root.get(),
                          host_impl_.active_tree());
}

TEST_F(TreeSynchronizerTest, SynchronizeAnimations) {
  LayerTreeSettings settings;
  FakeProxy proxy(scoped_ptr<Thread>(NULL));
  DebugScopedSetImplThread impl(&proxy);
  FakeRenderingStatsInstrumentation stats_instrumentation;
  scoped_ptr<LayerTreeHostImpl> host_impl =
      LayerTreeHostImpl::Create(settings,
                                NULL,
                                &proxy,
                                &stats_instrumentation);

  scoped_refptr<Layer> layer_tree_root = Layer::Create();

  layer_tree_root->SetLayerAnimationController(
      FakeLayerAnimationController::Create());

  EXPECT_FALSE(static_cast<FakeLayerAnimationController*>(
      layer_tree_root->layer_animation_controller())->SynchronizedAnimations());

  scoped_ptr<LayerImpl> layer_impl_tree_root =
      TreeSynchronizer::SynchronizeTrees(layer_tree_root.get(),
                                         scoped_ptr<LayerImpl>(),
                                         host_impl_.active_tree());
  TreeSynchronizer::PushProperties(layer_tree_root.get(),
                                   layer_impl_tree_root.get());
  layer_impl_tree_root =
      TreeSynchronizer::SynchronizeTrees(layer_tree_root.get(),
                                         layer_impl_tree_root.Pass(),
                                         host_impl_.active_tree());

  EXPECT_TRUE(static_cast<FakeLayerAnimationController*>(
      layer_tree_root->layer_animation_controller())->SynchronizedAnimations());
}

}  // namespace
}  // namespace cc
