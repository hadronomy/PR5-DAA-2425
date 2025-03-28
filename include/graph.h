#pragma once

#include <algorithm>
#include <cmath>
#include <concepts>
#include <functional>
#include <iostream>
#include <limits>
#include <optional>
#include <queue>
#include <sstream>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace daa {

// Define the concept first, before using it in forward declarations
// Concepts (C++20)
template <typename T>
concept Hashable = requires(T a) {
  { std::hash<T>{}(a) } -> std::convertible_to<std::size_t>;
};

// Forward declarations
template <typename V, typename E>
requires Hashable<V> class Graph;

template <typename V, typename E>
requires Hashable<V> class Vertex;

template <typename V, typename E>
requires Hashable<V> class Edge;

template <typename V, typename E>
requires Hashable<V> class Vertex {
 private:
  V data_;
  std::unordered_map<std::size_t, Edge<V, E>> edges_;
  std::size_t id_;

 public:
  explicit Vertex(V data, std::size_t id) : data_(std::move(data)), id_(id) {}

  // Accessors
  const V& data() const { return data_; }
  V& data() { return data_; }
  std::size_t id() const { return id_; }

  const std::unordered_map<std::size_t, Edge<V, E>>& edges() const { return edges_; }

  bool hasEdgeTo(std::size_t targetId) const { return edges_.find(targetId) != edges_.end(); }

  // For internal use by Graph - Modified to avoid default construction
  void addEdge(const Edge<V, E>& edge) {
    // Use insert_or_assign instead of operator[] to avoid default construction
    edges_.insert_or_assign(edge.targetId(), edge);
  }

  void removeEdge(std::size_t targetId) { edges_.erase(targetId); }

  std::optional<Edge<V, E>> getEdgeTo(std::size_t targetId) const {
    auto it = edges_.find(targetId);
    if (it != edges_.end()) {
      return it->second;
    }
    return std::nullopt;
  }
};

template <typename V, typename E>
requires Hashable<V> class Edge {
 private:
  std::size_t sourceId_;
  std::size_t targetId_;
  E data_;

 public:
  // Remove default constructor, it's no longer needed

  Edge(std::size_t sourceId, std::size_t targetId, E data = E{})
      : sourceId_(sourceId), targetId_(targetId), data_(std::move(data)) {}

  // Accessors
  std::size_t sourceId() const { return sourceId_; }
  std::size_t targetId() const { return targetId_; }

  const E& data() const { return data_; }
  E& data() { return data_; }
};

// GraphType specifies whether edges are directed or undirected
enum class GraphType { Directed, Undirected };

template <typename V, typename E = double>
requires Hashable<V> class Graph {
 private:
  std::unordered_map<std::size_t, Vertex<V, E>> vertices_;
  GraphType type_;
  std::size_t nextId_ = 0;

 public:
  explicit Graph(GraphType type = GraphType::Directed) : type_(type) {}

  // Vertex operations
  std::size_t addVertex(V data) {
    std::size_t id = nextId_++;
    vertices_.emplace(id, Vertex<V, E>{std::move(data), id});
    return id;
  }

  bool removeVertex(std::size_t id) {
    if (vertices_.find(id) == vertices_.end()) {
      return false;
    }

    // Remove all edges pointing to this vertex
    for (auto& [vid, vertex] : vertices_) {
      vertex.removeEdge(id);
    }

    // Remove the vertex itself
    vertices_.erase(id);
    return true;
  }

  bool hasVertex(std::size_t id) const { return vertices_.find(id) != vertices_.end(); }

  const Vertex<V, E>* getVertex(std::size_t id) const {
    auto it = vertices_.find(id);
    if (it != vertices_.end()) {
      return &it->second;
    }
    return nullptr;
  }

  Vertex<V, E>* getVertex(std::size_t id) {
    auto it = vertices_.find(id);
    if (it != vertices_.end()) {
      return &it->second;
    }
    return nullptr;
  }

  // Edge operations
  bool addEdge(std::size_t sourceId, std::size_t targetId, E data = E{}) {
    if (!hasVertex(sourceId) || !hasVertex(targetId)) {
      return false;
    }

    vertices_.at(sourceId).addEdge(Edge<V, E>{sourceId, targetId, data});

    // For undirected graphs, add the reverse edge as well
    if (type_ == GraphType::Undirected && sourceId != targetId) {
      vertices_.at(targetId).addEdge(Edge<V, E>{targetId, sourceId, data});
    }

    return true;
  }

  bool removeEdge(std::size_t sourceId, std::size_t targetId) {
    if (!hasVertex(sourceId) || !hasVertex(targetId)) {
      return false;
    }

    vertices_.at(sourceId).removeEdge(targetId);

    // For undirected graphs, remove the reverse edge as well
    if (type_ == GraphType::Undirected) {
      vertices_.at(targetId).removeEdge(sourceId);
    }

    return true;
  }

  bool hasEdge(std::size_t sourceId, std::size_t targetId) const {
    if (!hasVertex(sourceId)) {
      return false;
    }
    return vertices_.at(sourceId).hasEdgeTo(targetId);
  }

  std::optional<Edge<V, E>> getEdge(std::size_t sourceId, std::size_t targetId) const {
    if (!hasVertex(sourceId)) {
      return std::nullopt;
    }
    return vertices_.at(sourceId).getEdgeTo(targetId);
  }

  // Graph metadata
  std::size_t vertexCount() const { return vertices_.size(); }

  std::size_t edgeCount() const {
    std::size_t count = 0;
    for (const auto& [_, vertex] : vertices_) {
      count += vertex.edges().size();
    }
    return type_ == GraphType::Undirected ? count / 2 : count;
  }

  GraphType type() const { return type_; }

  // Iterators and accessors for all vertices/edges
  const std::unordered_map<std::size_t, Vertex<V, E>>& vertices() const { return vertices_; }

  // Utility methods
  std::vector<std::size_t> getVertexIds() const {
    std::vector<std::size_t> ids;
    ids.reserve(vertices_.size());
    for (const auto& [id, _] : vertices_) {
      ids.push_back(id);
    }
    return ids;
  }

  // Create a complete graph (useful for TSP)
  template <typename DistanceFunc>
  void makeComplete(DistanceFunc distanceFunc) {
    for (const auto& [sourceId, sourceVertex] : vertices_) {
      for (const auto& [targetId, targetVertex] : vertices_) {
        if (sourceId != targetId) {
          E weight = distanceFunc(sourceVertex.data(), targetVertex.data());
          addEdge(sourceId, targetId, weight);
        }
      }
    }
  }

  // Common algorithms
  std::vector<std::size_t> breadthFirstTraversal(std::size_t startId) const {
    if (!hasVertex(startId)) {
      return {};
    }

    std::vector<std::size_t> result;
    std::unordered_set<std::size_t> visited;
    std::queue<std::size_t> queue;

    visited.insert(startId);
    queue.push(startId);

    while (!queue.empty()) {
      std::size_t current = queue.front();
      queue.pop();
      result.push_back(current);

      const Vertex<V, E>& vertex = vertices_.at(current);
      for (const auto& [neighborId, _] : vertex.edges()) {
        if (visited.find(neighborId) == visited.end()) {
          visited.insert(neighborId);
          queue.push(neighborId);
        }
      }
    }

    return result;
  }

  // Serialization and deserialization
  std::string serialize() const {
    std::stringstream ss;
    ss << (type_ == GraphType::Directed ? "directed" : "undirected") << "\n";
    ss << vertices_.size() << "\n";

    // Write vertices
    for (const auto& [id, vertex] : vertices_) {
      ss << id << " " << serializeVertexData(vertex.data()) << "\n";
    }

    // Write edges
    for (const auto& [sourceId, vertex] : vertices_) {
      for (const auto& [targetId, edge] : vertex.edges()) {
        // For undirected graphs, only write edges where source < target
        if (type_ == GraphType::Directed || sourceId < targetId) {
          ss << sourceId << " " << targetId << " " << serializeEdgeData(edge.data()) << "\n";
        }
      }
    }

    return ss.str();
  }

  // Serialize in simple format (like 4.txt) for undirected graphs
  // Requires vertex data to be convertible to string for labels
  std::string serializeSimpleFormat() const {
    if (type_ != GraphType::Undirected) {
      throw std::runtime_error("Simple format serialization only supports undirected graphs");
    }

    std::stringstream ss;
    ss << vertices_.size() << "\n";

    // Write edges in format: VertexLabel1 VertexLabel2 Weight
    for (const auto& [sourceId, vertex] : vertices_) {
      for (const auto& [targetId, edge] : vertex.edges()) {
        // Only write each edge once (where source < target)
        if (sourceId < targetId) {
          const V& sourceLabel = vertex.data();
          const V& targetLabel = vertices_.at(targetId).data();
          ss << sourceLabel << " " << targetLabel << " " << edge.data() << "\n";
        }
      }
    }

    return ss.str();
  }

  // Deserialize from simple format (like 4.txt) for undirected graphs
  static Graph<V, E> deserializeSimpleFormat(const std::string& data) {
    std::stringstream ss(data);
    std::size_t vertexCount;
    ss >> vertexCount;

    Graph<V, E> graph(GraphType::Undirected);

    // Keep track of vertex label to ID mapping
    std::unordered_map<V, std::size_t> labelToId;

    // Read edges
    V sourceLabel, targetLabel;
    E weight;

    while (ss >> sourceLabel >> targetLabel >> weight) {
      // Add vertices if they don't exist
      if (labelToId.find(sourceLabel) == labelToId.end()) {
        labelToId[sourceLabel] = graph.addVertex(sourceLabel);
      }
      if (labelToId.find(targetLabel) == labelToId.end()) {
        labelToId[targetLabel] = graph.addVertex(targetLabel);
      }

      // Add the edge
      graph.addEdge(labelToId[sourceLabel], labelToId[targetLabel], weight);
    }

    return graph;
  }

  static Graph<V, E> deserialize(const std::string& data) {
    std::stringstream ss(data);
    std::string typeStr;
    ss >> typeStr;

    GraphType type = (typeStr == "directed") ? GraphType::Directed : GraphType::Undirected;

    Graph<V, E> graph(type);

    std::size_t vertexCount;
    ss >> vertexCount;

    // Read vertices
    for (std::size_t i = 0; i < vertexCount; ++i) {
      std::size_t id;
      ss >> id;

      V vertexData = deserializeVertexData(ss);
      graph.vertices_.emplace(id, Vertex<V, E>{std::move(vertexData), id});
      graph.nextId_ = std::max(graph.nextId_, id + 1);
    }

    // Read edges
    while (ss) {
      std::size_t sourceId, targetId;
      if (ss >> sourceId >> targetId) {
        E edgeData = deserializeEdgeData(ss);
        graph.addEdge(sourceId, targetId, edgeData);
      } else {
        break;
      }
    }

    return graph;
  }

  // TSP-specific utilities
  std::vector<std::size_t> getNearestNeighborPath(std::size_t startId) const {
    if (!hasVertex(startId) || vertices_.empty()) {
      return {};
    }

    std::vector<std::size_t> path;
    std::unordered_set<std::size_t> visited;

    std::size_t current = startId;
    path.push_back(current);
    visited.insert(current);

    while (visited.size() < vertices_.size()) {
      std::optional<std::size_t> nearest;
      E minDist = std::numeric_limits<E>::max();

      const Vertex<V, E>& vertex = vertices_.at(current);
      for (const auto& [neighborId, edge] : vertex.edges()) {
        if (visited.find(neighborId) == visited.end() && edge.data() < minDist) {
          minDist = edge.data();
          nearest = neighborId;
        }
      }

      if (!nearest) {
        // No unvisited neighbors, which shouldn't happen in a complete graph
        break;
      }

      current = *nearest;
      path.push_back(current);
      visited.insert(current);
    }

    return path;
  }

  std::vector<std::size_t> getMidNeighborPath(std::size_t startId) const {
    if (!hasVertex(startId) || vertices_.empty()) {
      return {};
    }

    std::vector<std::size_t> path;
    std::unordered_set<std::size_t> visited;

    std::size_t current = startId;
    path.push_back(current);
    visited.insert(current);

    while (visited.size() < vertices_.size()) {
      const Vertex<V, E>& vertex = vertices_.at(current);

      // Collect all unvisited neighbors and their distances
      std::vector<std::pair<std::size_t, E>> distances;
      for (const auto& [neighborId, edge] : vertex.edges()) {
        if (visited.find(neighborId) == visited.end()) {
          distances.emplace_back(neighborId, edge.data());
        }
      }

      if (distances.empty()) {
        break;
      }

      // Sort by distance
      std::sort(distances.begin(), distances.end(), [](const auto& a, const auto& b) {
        return a.second < b.second;
      });

      // Select the middle element
      size_t midIndex = distances.size() / 2;
      current = distances[midIndex].first;
      path.push_back(current);
      visited.insert(current);
    }

    return path;
  }

  E getPathCost(const std::vector<std::size_t>& path) const {
    if (path.size() <= 1) {
      return E{};
    }

    E totalCost = E{};

    for (std::size_t i = 0; i < path.size() - 1; ++i) {
      auto edge = getEdge(path[i], path[i + 1]);
      if (!edge) {
        throw std::runtime_error("Invalid path: no edge between vertices");
      }
      totalCost += edge->data();
    }

    // Add the cost of returning to the start (for TSP)
    auto lastEdge = getEdge(path.back(), path.front());
    if (lastEdge) {
      totalCost += lastEdge->data();
    }

    return totalCost;
  }

 private:
  // Serialization helpers - these can be specialized for custom vertex/edge types
  static std::string serializeVertexData(const V& data) {
    if constexpr (std::is_arithmetic_v<V>) {
      return std::to_string(data);
    } else {
      std::stringstream ss;
      ss << data;
      return ss.str();
    }
  }

  static std::string serializeEdgeData(const E& data) {
    if constexpr (std::is_arithmetic_v<E>) {
      return std::to_string(data);
    } else {
      std::stringstream ss;
      ss << data;
      return ss.str();
    }
  }

  static V deserializeVertexData(std::stringstream& ss) {
    if constexpr (std::is_arithmetic_v<V>) {
      V data;
      ss >> data;
      return data;
    } else {
      std::string data;
      ss >> data;
      std::stringstream dataStream(data);
      V result;
      dataStream >> result;
      return result;
    }
  }

  static E deserializeEdgeData(std::stringstream& ss) {
    if constexpr (std::is_arithmetic_v<E>) {
      E data;
      ss >> data;
      return data;
    } else {
      std::string data;
      ss >> data;
      std::stringstream dataStream(data);
      E result;
      dataStream >> result;
      return result;
    }
  }
};

}  // namespace daa