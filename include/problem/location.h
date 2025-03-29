#pragma once

#include <string>

#include "concepts.h"
#include "strong_types.h"

namespace daa {

class Location {
 public:
  class Builder;

 private:
  std::string id_;
  double x_;
  double y_;
  LocationType type_;
  std::string name_;
  Duration service_time_;
  Capacity waste_amount_;

 public:
  // Default constructor creates an empty location
  Location() noexcept
      : id_(""),
        x_(0.0),
        y_(0.0),
        type_(LocationType::DEPOT),
        name_(""),
        service_time_(0.0),
        waste_amount_(0.0) {}

  Location(
    std::string id,
    double x,
    double y,
    LocationType type,
    std::string name,
    Duration service_time,
    Capacity waste_amount
  )
      : id_(std::move(id)),
        x_(x),
        y_(y),
        type_(type),
        name_(std::move(name)),
        service_time_(service_time),
        waste_amount_(waste_amount) {}

  [[nodiscard]] const std::string& id() const noexcept { return id_; }
  [[nodiscard]] double x() const noexcept { return x_; }
  [[nodiscard]] double y() const noexcept { return y_; }
  [[nodiscard]] LocationType type() const noexcept { return type_; }
  [[nodiscard]] const std::string& name() const noexcept { return name_; }
  [[nodiscard]] const Duration& serviceTime() const noexcept { return service_time_; }
  [[nodiscard]] const Capacity& wasteAmount() const noexcept { return waste_amount_; }
};

class Location::Builder {
 private:
  std::string id_;
  double x_ = 0.0;
  double y_ = 0.0;
  LocationType type_ = LocationType::DEPOT;
  std::string name_;
  Duration service_time_{0.0};
  Capacity waste_amount_{0.0};

 public:
  Builder& setId(std::string id) {
    id_ = std::move(id);
    return *this;
  }

  Builder& setCoordinates(double x, double y) {
    x_ = x;
    y_ = y;
    return *this;
  }

  Builder& setType(LocationType type) {
    type_ = type;
    return *this;
  }

  Builder& setName(std::string name) {
    name_ = std::move(name);
    return *this;
  }

  Builder& setServiceTime(Duration time) {
    service_time_ = time;
    return *this;
  }

  Builder& setWasteAmount(Capacity amount) {
    waste_amount_ = amount;
    return *this;
  }

  [[nodiscard]] Location build() const {
    return Location(id_, x_, y_, type_, name_, service_time_, waste_amount_);
  }
};

}  // namespace daa
