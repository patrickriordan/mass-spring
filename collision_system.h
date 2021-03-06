#ifndef COLLISION_SYSTEM_H__
#define COLLISION_SYSTEM_H__
#include "../Eigen/Core"
#include <vector>

class CollisionSystem {
 public:
  CollisionSystem();
  ~CollisionSystem();
  void GetCollisions(std::vector<unsigned int>& vertexToFace, std::vector<unsigned int>& edgeToEdge, std::vector<float>& veToFaTime, std::vector<float>& edToEdTime);
  void UpdateVertex(unsigned int index, const Eigen::Vector3d& vec);
  void InitSystem(const std::vector<Eigen::Vector3d>& verts, const std::vector<int>& tris);
};
#endif
