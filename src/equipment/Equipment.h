#pragma once

#include "scene/Transform.h"

#include <string>

namespace mine {

enum class EquipmentKind { Shearer, Support, Conveyor, Light };

class Equipment {
public:
    Equipment(int id, EquipmentKind kind, std::string name)
        : id_(id), kind_(kind), name_(std::move(name)) {}
    virtual ~Equipment() = default;

    Equipment(const Equipment&) = delete;
    Equipment& operator=(const Equipment&) = delete;
    Equipment(Equipment&&) = default;
    Equipment& operator=(Equipment&&) = default;

    [[nodiscard]] int id() const { return id_; }
    [[nodiscard]] EquipmentKind kind() const { return kind_; }
    [[nodiscard]] const std::string& name() const { return name_; }
    [[nodiscard]] Transform& transform() { return transform_; }
    [[nodiscard]] const Transform& transform() const { return transform_; }

protected:
    int id_;
    EquipmentKind kind_;
    std::string name_;
    Transform transform_;
};

} // namespace mine
