// Copyright (c) 2020 The Orbit Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/*
 * Bridging accessibility from OrbitQt and OrbitGl.
 *
 * For a detailed documentation on this, check AccessibilityAdapter.cpp - unless changes to the
 * accessibility interaction between OrbitQt and OrbitGl are required, there should be no need to
 * deal with these definitions.
 */

#ifndef ORBIT_QT_ACCESSIBILITY_ADAPTER_H_
#define ORBIT_QT_ACCESSIBILITY_ADAPTER_H_

#include <absl/base/casts.h>
#include <absl/container/flat_hash_map.h>
#include <absl/container/flat_hash_set.h>

#include <QAccessible>
#include <QAccessibleWidget>
#include <QWidget>

#include "OrbitBase/Logging.h"
#include "OrbitGlAccessibility.h"
#include "orbitglwidget.h"

namespace orbit_qt {

/*
 * Instances of this act as adapters from QAccessibleInterface to orbit_gl::GlAccessibleInterface
 * and vice versa. Static methods of AccessibilityAdapter provide factory methods and tracking of
 * adapters.
 *
 * See file documentation above for more details.
 */
class AccessibilityAdapter : public QAccessibleInterface {
 public:
  AccessibilityAdapter() = delete;
  AccessibilityAdapter(const AccessibilityAdapter& rhs) = delete;
  AccessibilityAdapter(AccessibilityAdapter&& rhs) = delete;
  AccessibilityAdapter& operator=(const AccessibilityAdapter& rhs) = delete;
  AccessibilityAdapter& operator=(AccessibilityAdapter&& rhs) = delete;

  bool isValid() const override {
    bool result = info_ != nullptr;
    CHECK(!result || interface_map_.find(info_)->second == this);
    return result;
  }
  QObject* object() const override { return &dummy_; }
  QAccessibleInterface* focusChild() const override { return nullptr; }

  QAccessibleInterface* parent() const override {
    return GetOrCreateAdapter(info_->AccessibleParent());
  }
  QAccessibleInterface* child(int index) const override {
    return GetOrCreateAdapter(info_->AccessibleChild(index));
  }
  int childCount() const override { return info_->AccessibleChildCount(); }
  int indexOfChild(const QAccessibleInterface* child) const override;
  QAccessibleInterface* childAt(int x, int y) const override;

  QString text(QAccessible::Text /*t*/) const override {
    return QString::fromStdString(info_->AccessibleName());
  }
  void setText(QAccessible::Text /*t*/, const QString& /*text*/) override{};
  QRect rect() const override;
  QAccessible::Role role() const override;

  virtual QAccessible::State state() const override {
    static_assert(sizeof(QAccessible::State) == sizeof(orbit_gl::AccessibilityState));
    return absl::bit_cast<QAccessible::State>(info_->AccessibleState());
  }

  static QAccessibleInterface* GetOrCreateAdapter(const orbit_gl::GlAccessibleInterface* iface);
  static void RegisterAdapter(const orbit_gl::GlAccessibleInterface* gl_control,
                              QAccessibleInterface* qt_control) {
    interface_map_.emplace(gl_control, qt_control);
  }
  /* Called when a QAccessibleInterface which has been registered through "RegisterAdapter",
  but not created by this class, is deleted. Should only be needed for OrbitGlWidgets.*/
  static void QAccessibleDeleted(QAccessibleInterface* iface);

  static int RegisteredAdapterCount() { return interface_map_.size(); }

 private:
  mutable QObject dummy_;
  explicit AccessibilityAdapter(const orbit_gl::GlAccessibleInterface* info) : info_(info){};

  const orbit_gl::GlAccessibleInterface* info_;

  static absl::flat_hash_map<const orbit_gl::GlAccessibleInterface*, QAccessibleInterface*>
      interface_map_;
  static absl::flat_hash_set<const QAccessibleInterface*> managed_adapters_;

  static void Init();
  static void OnInterfaceDeleted(orbit_gl::GlAccessibleInterface* iface);
};

void InstallAccessibilityFactories();

}  // namespace orbit_qt

#endif