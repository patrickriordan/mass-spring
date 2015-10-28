#include <GLFW/glfw3.h>
#include "particle_system.h"
#include "draw_delegate.h"
#include "meshgen.h"
#include "Eigen/Sparse"
#include "Eigen/Dense"
#include "Eigen/IterativeLinearSolvers"
#include <iostream>
#include <math.h>
#include "collision_system.h"

ParticleSystem::ParticleSystem() {
  stiffness = 1000;
  dampness = 10;
  gravity = 9.8;
  groundLevel = 5;
  groundStiffness = 1000;
  mouseStiffness = 100;
  colSys = new CollisionSystem();
  useColSys = false;
}

ParticleSystem::~ParticleSystem() {
  delete colSys;
}

void ParticleSystem::Update(double timestep, bool solveWithguess, bool coro, int groundMode) {
  // Always solveWithguess
  // coro means whether to use corotational linear FEM or normal linear FEM
  corotational = coro;
  ImplicitEulerSparse(timestep);
  //ExplicitEuler(timestep);

  if (useColSys) {
    for (int i = 0; i < outsidePoints.size(); i++) {
      if (outsidePoints[i] > 0) {
        colSys->UpdateVertex(i, particles[outsidePoints[i]].x);
      }
    }
    std::vector<unsigned int> vertexToFace;
    colSys->GetCollisions(vertexToFace);
    if (vertexToFace.size() > 20) {
      //printf("Too many collisions\n");
      //exit(0);
    }
    for (int i = 0; i < vertexToFace.size(); i += 2) {
      // calculate normal of tri
      Particle *p1, *p2, *p3, *v1;
      int p1_i, p2_i, p3_i, v1_i;
      p1_i = outsidePoints[faceToOut[3 * vertexToFace[i + 1]]];
      p2_i = outsidePoints[faceToOut[3 * vertexToFace[i + 1] + 1]];
      p3_i = outsidePoints[faceToOut[3 * vertexToFace[i + 1] + 2]];
      v1_i = outsidePoints[vertexToFace[i]];
      if (p1_i < 0 && v1_i >= 0) {
        GetPointP(p1_i, p1);
        GetPointP(p2_i, p2);
        GetPointP(p3_i, p3);
        GetPointP(v1_i, v1);
        Eigen::Vector3d temp1, temp2;
        temp1 = p2->x - p1->x;
        temp2 = p3->x - p1->x;
        temp1 = temp1.cross(temp2);
        temp1.normalize();
        temp1;
        // Project vertex onto plane
        double d = p1->x.dot(temp1);
        double v = (d - (v1->x.dot(temp1)));
        if (v < 0) {
          //printf("inside\n");
        }
        Eigen::Vector3d planePoint = v1->x + v * temp1;
        //printf("Original point: %f, %f, %f\n", v1->x[0], v1->x[1], v1->x[2]);
        //printf("Plane point: %f, %f, %f\n", planePoint[0], planePoint[1], planePoint[2]);
        //printf("V: %f\n", v);
        //printf("temp1: %f, %f, %f\n", temp1[0], temp1[1], temp1[2]);
        planePoint +=  temp1 * .05 * 30 * timestep * timestep;
        v1->x[0] = planePoint[0];
        v1->x[1] = planePoint[1];
        v1->x[2] = planePoint[2];
        //v1->x[0] -= v1->v[0] * timestep;
        //v1->x[1] -= v1->v[1] * timestep;
        //v1->x[2] -= v1->v[2] * timestep;
        v1->v[0] = 0;
        v1->v[1] = 0;
        v1->v[2] = 0;
        colSys->UpdateVertex(vertexToFace[i], v1->x);
      }
    }
    vertexToFace.clear();
    colSys->GetCollisions(vertexToFace);

//Eigen::Vector3d newVelocity = (p1->v * bary[0] + p2->v * bary[1] + p3->v * bary[2]) / 2;
//v1->x = planePoint;
//v1->v = newVelocity;

      //if (p1_i >= 0 && p2_i >= 0 && p3_i >= 0 && v1_i >= 0) {
      //  p1 = &(particles[p1_i]);
      //  p2 = &(particles[p2_i]);
      //  p3 = &(particles[p3_i]);
      //  v1 = &(particles[v1_i]);
      //  Eigen::Vector3d temp1, temp2;
      //  temp1 = p2->x - p1->x;
      //  temp2 = p3->x - p1->x;
      //  temp1 = temp1.cross(temp2);
      //  temp1.normalize();
      //  temp1;
      //  // Project vertex onto plane
      //  double d = p1->x.dot(temp1);
      //  double v = (d - (v1->x.dot(temp1)));
      //  Eigen::Vector3d planePoint = v1->x + v * temp1;
      //  printf("Original point: %f, %f, %f\n", v1->x[0], v1->x[1], v1->x[2]);
      //  printf("Plane point: %f, %f, %f\n", planePoint[0], planePoint[1], planePoint[2]);
      //  printf("V: %f\n", v);
      //  printf("temp1: %f, %f, %f\n", temp1[0], temp1[1], temp1[2]);
      //  Eigen::Matrix3d baryM;
      //  baryM << p1->x[0], p2->x[0], p3->x[0],
      //           p1->x[1], p2->x[1], p3->x[1],
      //           p1->x[2], p2->x[2], p3->x[2];

      //  Eigen::Vector3d bary = baryM.inverse() * planePoint;
      //  printf("Sum of barys: %f\n", bary[0] + bary[1] + bary[2]);
      //  Eigen::Vector3d newVelocity = (v1->v + p1->v * bary[0] + p2->v * bary[1] + p3->v * bary[2]) / 2;
      //  v1->x = (v1->x + planePoint)/2;
      //  Eigen::Vector3d moveTri = 3 * (v1->x - planePoint);
      //  v1->v = newVelocity;
      //  p1->v = newVelocity * bary[0] + p1->v * (1 - bary[0]);
      //  p2->v = newVelocity * bary[1] + p2->v * (1 - bary[1]);
      //  p3->v = newVelocity * bary[2] + p3->v * (1 - bary[2]);
      //  p1->x += moveTri * bary[0];
      //  p2->x += moveTri * bary[1];
      //  p3->x += moveTri * bary[2];

      //  //Eigen::Vector3d newVelocity = (p1->v * bary[0] + p2->v * bary[1] + p3->v * bary[2]) / 2;
      //  //v1->x = planePoint;
      //  //v1->v = newVelocity;

      //  //colSys->UpdateVertex(vertexToFace[i], v1->x);
      //  //colSys->UpdateVertex(faceToOut[vertexToFace[i + 1]], p1->x);
      //  //colSys->UpdateVertex(faceToOut[vertexToFace[i + 1] + 1], p2->x);
      //  //colSys->UpdateVertex(faceToOut[vertexToFace[i + 1] + 2], p3->x);
      //  // for now just move the vertex to match the face
      //  // calculate barycentric coordinates of hit
      //}
  //  }
  //}
  }
  // Optionally make things bounce of the ground
  groundMode = 0;
  switch (groundMode) {
    case 0:
      break;
    case 1:
      // Penalty method
      for (int i = 0; i < outsidePoints.size(); i++) {
        if (outsidePoints[i] > -1) {
          int point = outsidePoints[i];
          if (particles[point].x[1] > groundLevel) {
            particles[point].f[1] -= groundStiffness * (particles[point].x[1] - groundLevel) * timestep;
          }
        }
      }
      break;
    case 2:
      // Snap to floor and penalty
      for (int i = 0; i < outsidePoints.size(); i++) {
        if (outsidePoints[i] > -1) {
          int point = outsidePoints[i];
          if (particles[point].x[1] > groundLevel) {
            particles[point].f[1] -= groundStiffness * (particles[point].x[1] - groundLevel) * timestep;
            particles[point].x[1] = groundLevel;
          }
        }
      }
      break;
    case 3:
      //Snap to prev intersection with ground and ground normal penalty
      for (int i = 0; i < outsidePoints.size(); i++) {
        if (outsidePoints[i] > -1) {
          int point = outsidePoints[i];
          if (particles[point].x[1] > groundLevel) {
            double part = (particles[point].x[1] - groundLevel)/particles[point].v[1];
            particles[point].f[1] -= groundStiffness * (particles[point].x[1] - groundLevel) * timestep;
            particles[point].x = particles[point].x - part * particles[point].v;
            particles[point].v[1] = 0;
          }
        }
      }
      break;
    case 4:
      //Snap to prev intersection with ground and ground normal penalty plus friction
      for (int i = 0; i < outsidePoints.size(); i++) {
        if (outsidePoints[i] > -1) {
          int point = outsidePoints[i];
          if (particles[point].x[1] > groundLevel) {
            double part = (particles[point].x[1] - groundLevel)/particles[point].v[1];
            particles[point].f[1] -= groundStiffness * (particles[point].x[1] - groundLevel) * timestep;
            particles[point].f[0] -= part * particles[point].v[0] * timestep * groundStiffness;
            particles[point].f[2] -= part * particles[point].v[2] * timestep * groundStiffness;
            particles[point].x[1] = groundLevel;
          }
        }
      }
    case 5:
      //Snap to floor and infinite friction
      for (int i = 0; i < outsidePoints.size(); i++) {
        if (outsidePoints[i] > -1) {
          int point = outsidePoints[i];
          if (particles[point].x[1] > groundLevel) {
            particles[point].x[1] = groundLevel;
            particles[point].v[0] = 0;
            particles[point].v[1] = 0;
            particles[point].v[2] = 0;
          }
        }
      }
      break;
    case 6:
      //Implicit penalty and snap to floor
      for (int i = 0; i < outsidePoints.size(); i++) {
        if (outsidePoints[i] > -1) {
          int point = outsidePoints[i];
          if (particles[point].x[1] > groundLevel) {
            particles[point].v[1] = ((1/particles[point].iMass) * particles[point].v[1] - groundStiffness * timestep * (particles[point].x[1] - groundLevel)) /
              (1/particles[point].iMass + timestep * timestep * groundStiffness);
            particles[point].v[0] = 0;
            particles[point].v[2] = 0;
            particles[point].x[1] = groundLevel;
          }
        }
      }
      break;
  }
}
// fun code from http://www.gamedev.net/topic/142760-the-fasted-raytriangle-collision-detection/

#define EPSILON 0.000001
#define CROSS(dest,v1,v2) \
          dest[0]=v1[1]*v2[2]-v1[2]*v2[1]; \
          dest[1]=v1[2]*v2[0]-v1[0]*v2[2]; \
          dest[2]=v1[0]*v2[1]-v1[1]*v2[0];
#define DOT(v1,v2) (v1[0]*v2[0]+v1[1]*v2[1]+v1[2]*v2[2])
#define SUB(dest,v1,v2) \
          dest[0]=v1[0]-v2[0]; \
          dest[1]=v1[1]-v2[1]; \
          dest[2]=v1[2]-v2[2]; 
static int
intersect_triangle(Eigen::Vector3d orig, Eigen::Vector3d dir,
                   Eigen::Vector3d vert0, Eigen::Vector3d vert1, Eigen::Vector3d vert2,
                   double *t, double *u, double *v)
{
   double edge1[3], edge2[3], tvec[3], pvec[3], qvec[3];
   double det,inv_det;

   /* find vectors for two edges sharing vert0 */
   SUB(edge1, vert1, vert0);
   SUB(edge2, vert2, vert0);

   /* begin calculating determinant - also used to calculate U parameter */
   CROSS(pvec, dir, edge2);

   /* if determinant is near zero, ray lies in plane of triangle */
   det = DOT(edge1, pvec);

   if (det > -EPSILON && det < EPSILON)
     return 0;
   inv_det = 1.0 / det;

   /* calculate distance from vert0 to ray origin */
   SUB(tvec, orig, vert0);

   /* calculate U parameter and test bounds */
   *u = DOT(tvec, pvec) * inv_det;
   if (*u < 0.0 || *u > 1.0)
     return 0;

   /* prepare to test V parameter */
   CROSS(qvec, tvec, edge1);

   /* calculate V parameter and test bounds */
   *v = DOT(dir, qvec) * inv_det;
   if (*v < 0.0 || *u + *v > 1.0)
     return 0;

   /* calculate t, ray intersects triangle */
   *t = DOT(edge2, qvec) * inv_det;
   return 1;
}





int lastpoint = -1;

void ParticleSystem::onMouseDrag(Eigen::Vector3d ori, Eigen::Vector3d ray, double timestep) {
  ray.normalize();
  int point = lastpoint;
  if (lastpoint >= 0) {
    double temp0 = ray.dot(particles[point].x - ori);
    Eigen::Vector3d closePoint = ori + temp0 * ray;
    Eigen::Vector3d moveDir = ori + temp0 * ray -  particles[point].x;
    double dist = moveDir.norm();
    //moveDir.normalize();
    particles[point].f += moveDir * mouseStiffness * timestep;
    //Spring to cursor?
  }
}
void ParticleSystem::onMousePress(Eigen::Vector3d ori, Eigen::Vector3d ray) {
  ray.normalize();
  double closestDistance = -1;
  int curPoint = -1;
  int closeCount = 0;
  for (int i = 0; i < faces.size(); i += 3) {
    double t, u, v;
    Particle*a,*b,*c;
    GetPointP(faces[i], a);
    GetPointP(faces[i + 1], b);
    GetPointP(faces[i + 2], c);
    if (intersect_triangle(ori, ray, a->x, b->x, c->x, &t, &u, &v)) {
      int point;
      if (1 - u - v > u) {
        if (1 - u - v > v) {
          point = faces[i];
        } else {
          point = faces[i + 2];
        }
      } else {
        if (u > v) {
          point = faces[i + 1];
        } else {
          point = faces[i + 2];
        }
      }
      if (t >= 0) {
        closeCount += 1;
        if (closestDistance < 0 || t < closestDistance) {
          curPoint = point;
          closestDistance = t;
        }
      }
    }
      //point = faces[i];
      //int point = outsidePoints[i];
      //double temp0 = ray.dot(particles[point].x - ori);
      //double distance = (particles[point].x - (ori + temp0 * ray)).norm();
      //if (distance < .5) {
      //  closeCount += 1;
      //  if (closestDistance < 0 || temp0 + distance < closestDistance) {
      //    curPoint = point;
      //  }
      //}
  }
          fprintf(stderr, "New lastpoint: %i\n", curPoint);
  lastpoint = curPoint;
}
// Help function to show strain properly through color
static void LerpColors(double strain, float*color3) {
   if (strain < 1) {
      *(color3) = 0;
      *(color3+1) = 0;
      *(color3+2) = strain;
   } else if (strain < 2) {
      *(color3) = strain - 1;
      *(color3+1) = 0;
      *(color3+2) = strain;
   } else {
      *(color3) = 1;
      *(color3+1) = 0;
      *(color3+2) = 1 - (strain -2);
   }
}

// Get the center of all tets
float* ParticleSystem::GetTetCenter(int* size) {
  *size = tets.size() * 3;
  posTemp.resize(*size);
  for (int i = 0; i < *size/3; i++) {
    Particle *p1, *p2, *p3, *p4;
    GetTetP(i, p1, p2, p3, p4);
    Eigen::Vector3d avg = (p1->x + p2->x + p3->x + p4->x)/4;
    posTemp[i*3] = avg[0];
    posTemp[i*3+1] = avg[1];
    posTemp[i*3+2] = avg[2];
  }
  return posTemp.data();
}

// Get the strain colors of all tets
float* ParticleSystem::GetCenterColors(int*size, double strainSize) {
  *size = tets.size() * 3;
  colorTemp.resize(*size);
  for (int i = 0; i < *size/3; i++) {
    Particle *p1, *p2, *p3, *p4;
    GetTetP(i, p1, p2, p3, p4);
    Eigen::Matrix3d temp;
    temp << p2->x - p1->x, p3->x - p1->x, p4->x - p1->x;
    Eigen::Matrix3d deformGradient = (temp * tets[i].inversePos) - Eigen::Matrix3d::Identity();
    Eigen::Matrix3d greenStrainTensor;
    if (corotational) {
      greenStrainTensor = .5 * (deformGradient + deformGradient.transpose() +
                                                deformGradient.transpose() * deformGradient);
    } else {
      greenStrainTensor = .5 * (deformGradient + deformGradient.transpose());
    }
    double v = .4;
    Eigen::VectorXd strainVec(6);
    strainVec << greenStrainTensor(0,0), greenStrainTensor(1,1),
                                    greenStrainTensor(2,2), greenStrainTensor(1,0),
                                    greenStrainTensor(1,2), greenStrainTensor(2,0);

    Eigen::MatrixXd strainToStress(6,6);
    strainToStress << 1 - v, v, v, 0, 0, 0,
                                           v, 1 - v, v, 0, 0, 0,
                                           v, v, 1 - v, 0, 0, 0,
                                           0, 0, 0, 1 - 2*v, 0, 0,
                                           0, 0, 0, 0, 1 - 2*v, 0,
                                           0, 0, 0, 0, 0, 1 - 2*v;
    Eigen::VectorXd stressVec(6);
    stressVec = (tets[i].k/((1 + v) * (1 - 2*v))) * strainToStress * strainVec;
    Eigen::Matrix3d stressTensor;
    stressTensor << stressVec[0], stressVec[3], stressVec[5],
                    stressVec[3], stressVec[1], stressVec[4],
                    stressVec[5], stressVec[4], stressVec[2];
    double strain = stressTensor.norm();

    LerpColors(strain * strainSize, &(colorTemp[i*3]));
  }
  return colorTemp.data();
}

// Get colors for lines
float* ParticleSystem::GetColors(int* size, double strainSize, float xpos, float ypos, float zpos) {
  *size = tets.size()* 6 * 3 * 2;
  int perTet = 6 * 3 * 2;
  colorTemp.resize(*size);
  Eigen::Vector3d campos;
  campos << xpos, ypos, zpos;
  for (int i = 0; i < *size/3; i++) {
    Eigen::Vector3d ppos;
    ppos << posTemp[i*3], posTemp[i*3+1], posTemp[i*3+2];
    double color = 1/(ppos - campos).norm();

    colorTemp[i*3] = 0;//color + (i%2)/2.0;
    colorTemp[i*3 + 1] = 0;//color;
    colorTemp[i*3 + 2] = 0;//color + ((i+1)%2)/2.0;
    if (i%12 == 0 && tets[i/12].to[0] == lastpoint ||
        i%12 == 1 && tets[i/12].to[1] == lastpoint ||
        i%12 == 2 && tets[i/12].to[0] == lastpoint ||
        i%12 == 3 && tets[i/12].to[2] == lastpoint ||
        i%12 == 4 && tets[i/12].to[0] == lastpoint ||
        i%12 == 5 && tets[i/12].to[3] == lastpoint ||
        i%12 == 6 && tets[i/12].to[1] == lastpoint ||
        i%12 == 7 && tets[i/12].to[2] == lastpoint ||
        i%12 == 8 && tets[i/12].to[1] == lastpoint ||
        i%12 == 9 && tets[i/12].to[3] == lastpoint ||
        i%12 == 10 && tets[i/12].to[2] == lastpoint ||
        i%12 == 11 && tets[i/12].to[3] == lastpoint) {
        colorTemp[i*3] = 1;
        colorTemp[i*3 + 1] = 0;
        colorTemp[i*3 + 2] = 0;
    }

    //colorTemp[i*6 + 3] = 1;
    //colorTemp[i*6 + 4] = 0;
    //colorTemp[i*6 + 5] = 0;
  }
  return colorTemp.data();
}

// Get positions for lines
float* ParticleSystem::GetPositions3d(int* size) {
  *size = tets.size()* 6 * 3 * 2;
  int perTet = 6 * 3 * 2;
  posTemp.resize(*size);
  for (int i = 0; i < tets.size(); i++) {
    Particle *p1, *p2, *p3, *p4;
    GetTetP(i, p1, p2, p3, p4);
    int c = 0;
    // p1 to p2
    posTemp[i*perTet + c++] = ((float)p1->x[0]);
    posTemp[i*perTet + c++] = ((float)p1->x[1]);
    posTemp[i*perTet + c++] = ((float)p1->x[2]);

    posTemp[i*perTet + c++] = ((float)p2->x[0]);
    posTemp[i*perTet + c++] = ((float)p2->x[1]);
    posTemp[i*perTet + c++] = ((float)p2->x[2]);

    // p1 to p3
    posTemp[i*perTet + c++] = ((float)p1->x[0]);
    posTemp[i*perTet + c++] = ((float)p1->x[1]);
    posTemp[i*perTet + c++] = ((float)p1->x[2]);

    posTemp[i*perTet + c++] = ((float)p3->x[0]);
    posTemp[i*perTet + c++] = ((float)p3->x[1]);
    posTemp[i*perTet + c++] = ((float)p3->x[2]);

    // p1 to p4
    posTemp[i*perTet + c++] = ((float)p1->x[0]);
    posTemp[i*perTet + c++] = ((float)p1->x[1]);
    posTemp[i*perTet + c++] = ((float)p1->x[2]);

    posTemp[i*perTet + c++] = ((float)p4->x[0]);
    posTemp[i*perTet + c++] = ((float)p4->x[1]);
    posTemp[i*perTet + c++] = ((float)p4->x[2]);

    // p2 to p3
    posTemp[i*perTet + c++] = ((float)p2->x[0]);
    posTemp[i*perTet + c++] = ((float)p2->x[1]);
    posTemp[i*perTet + c++] = ((float)p2->x[2]);

    posTemp[i*perTet + c++] = ((float)p3->x[0]);
    posTemp[i*perTet + c++] = ((float)p3->x[1]);
    posTemp[i*perTet + c++] = ((float)p3->x[2]);

    // p2 to p4
    posTemp[i*perTet + c++] = ((float)p2->x[0]);
    posTemp[i*perTet + c++] = ((float)p2->x[1]);
    posTemp[i*perTet + c++] = ((float)p2->x[2]);

    posTemp[i*perTet + c++] = ((float)p4->x[0]);
    posTemp[i*perTet + c++] = ((float)p4->x[1]);
    posTemp[i*perTet + c++] = ((float)p4->x[2]);

    // p3 to p4
    posTemp[i*perTet + c++] = ((float)p3->x[0]);
    posTemp[i*perTet + c++] = ((float)p3->x[1]);
    posTemp[i*perTet + c++] = ((float)p3->x[2]);

    posTemp[i*perTet + c++] = ((float)p4->x[0]);
    posTemp[i*perTet + c++] = ((float)p4->x[1]);
    posTemp[i*perTet + c++] = ((float)p4->x[2]);
  }
  return posTemp.data();
}

// Get triangler colors, its just a light calculation
float* ParticleSystem::GetTriColors(int* size, double strainSize) {
  *size = faces.size()*3;
  colorTemp.resize(*size);
  int perTri = 9;
  Eigen::Vector3d light1,light2;
  light1 << 1, 1, -1;
  light1.normalize();
  light2 << -1, 1, -1;
  light2.normalize();
  for (int i = 0; i < *size/9; i++) {
    int c = 0;
    Eigen::Vector3d p0, p1, p2;
    p0 << posTemp[i*perTri + c++], posTemp[i*perTri + c++], posTemp[i*perTri + c++];
    p1 << posTemp[i*perTri + c++], posTemp[i*perTri + c++], posTemp[i*perTri + c++];
    p2 << posTemp[i*perTri + c++], posTemp[i*perTri + c++], posTemp[i*perTri + c++];
    p1 -= p0;
    p2 -= p0;
    p0 = p1.cross(p2);
    p0.normalize();
    double color1 = p0.dot(light1);
    double color2 = 0;//.5*p0.dot(light2);
    int c2 = 0;
    colorTemp[i*perTri + c2++] = color1+color2;
    colorTemp[i*perTri + c2++] = color1+color2;
    colorTemp[i*perTri + c2++] = color1+color2;

    colorTemp[i*perTri + c2++] = color1+color2;
    colorTemp[i*perTri + c2++] = color1+color2;
    colorTemp[i*perTri + c2++] = color1+color2;

    colorTemp[i*perTri + c2++] = color1+color2;
    colorTemp[i*perTri + c2++] = color1+color2;
    colorTemp[i*perTri + c2++] = color1+color2;
    //LerpColors((i/(4*3))/10, &(colorTemp[i*3])); 
  }
  return colorTemp.data();
}

// Get the strain for the surfarce of tris
float* ParticleSystem::GetStrainSurfaceTriColors(int* size, double strainSize) {
  *size = faces.size()*3;
  colorTemp.resize(*size);
  for (int i = 0; i < tets.size(); i++) {
    Particle *p1,*p2,*p3,*p4;
    GetTetP(i, p1, p2, p3, p4);

    Eigen::Matrix3d temp;
    temp << p2->x - p1->x, p3->x - p1->x, p4->x - p1->x;
    Eigen::Matrix3d deformGradient = (temp * tets[i].inversePos) - Eigen::Matrix3d::Identity();
    Eigen::Matrix3d greenStrainTensor;
    if (corotational) {
      greenStrainTensor = .5 * (deformGradient + deformGradient.transpose() +
                                                deformGradient.transpose() * deformGradient);
    } else {
      greenStrainTensor = .5 * (deformGradient + deformGradient.transpose());
    }
    double v = .4;
    Eigen::VectorXd strainVec(6);
    strainVec << greenStrainTensor(0,0), greenStrainTensor(1,1),
                                    greenStrainTensor(2,2), greenStrainTensor(1,0),
                                    greenStrainTensor(1,2), greenStrainTensor(2,0);

    Eigen::MatrixXd strainToStress(6,6);
    strainToStress << 1 - v, v, v, 0, 0, 0,
                                           v, 1 - v, v, 0, 0, 0,
                                           v, v, 1 - v, 0, 0, 0,
                                           0, 0, 0, 1 - 2*v, 0, 0,
                                           0, 0, 0, 0, 1 - 2*v, 0,
                                           0, 0, 0, 0, 0, 1 - 2*v;
    Eigen::VectorXd stressVec(6);
    stressVec = (tets[i].k/((1 + v) * (1 - 2*v))) * strainToStress * strainVec;
    Eigen::Matrix3d stressTensor;
    stressTensor << stressVec[0], stressVec[3], stressVec[5],
                    stressVec[3], stressVec[1], stressVec[4],
                    stressVec[5], stressVec[4], stressVec[2];
    double strain = stressTensor.norm();
    tets[i].strain = strain;
  }
  for (int i = 0; i < faces.size()/3; i++) {
    double strain = tets[facetotet[i]].strain;
    LerpColors(strain * strainSize, &(colorTemp[i*9]));
    LerpColors(strain * strainSize, &(colorTemp[i*9+3]));
    LerpColors(strain * strainSize, &(colorTemp[i*9+6]));
  }
  return colorTemp.data();
}

float* ParticleSystem::GetSurfaceTriangles3d(int* size) {
  *size = faces.size()*3;
  posTemp.resize(*size);
  for (int i = 0; i < faces.size(); i++) {
    Particle* p1;
    GetPointP(faces[i], p1);
    posTemp[i*3] = p1->x[0];
    posTemp[i*3+1] = p1->x[1];
    posTemp[i*3+2] = p1->x[2];
 }
 return posTemp.data();
}

float* ParticleSystem::GetAllTriangles3d(int* size) {
  *size = tets.size()* 4 * 3 * 3;
  int perTet = 4 * 3 * 3;
  posTemp.resize(*size);
  for (int i = 0; i < tets.size(); i++) {
    Particle *p1, *p2, *p3, *p4;
    GetTetP(i, p1, p2, p3, p4);
    int c = 0;
    // p1 p2 p3
    posTemp[i*perTet + c++] = ((float)p1->x[0]);
    posTemp[i*perTet + c++] = ((float)p1->x[1]);
    posTemp[i*perTet + c++] = ((float)p1->x[2]);

    posTemp[i*perTet + c++] = ((float)p2->x[0]);
    posTemp[i*perTet + c++] = ((float)p2->x[1]);
    posTemp[i*perTet + c++] = ((float)p2->x[2]);

    posTemp[i*perTet + c++] = ((float)p3->x[0]);
    posTemp[i*perTet + c++] = ((float)p3->x[1]);
    posTemp[i*perTet + c++] = ((float)p3->x[2]);

    // p1 p4 p2
    posTemp[i*perTet + c++] = ((float)p1->x[0]);
    posTemp[i*perTet + c++] = ((float)p1->x[1]);
    posTemp[i*perTet + c++] = ((float)p1->x[2]);

    posTemp[i*perTet + c++] = ((float)p4->x[0]);
    posTemp[i*perTet + c++] = ((float)p4->x[1]);
    posTemp[i*perTet + c++] = ((float)p4->x[2]);

    posTemp[i*perTet + c++] = ((float)p2->x[0]);
    posTemp[i*perTet + c++] = ((float)p2->x[1]);
    posTemp[i*perTet + c++] = ((float)p2->x[2]);

    // p1 p3 p4
    posTemp[i*perTet + c++] = ((float)p1->x[0]);
    posTemp[i*perTet + c++] = ((float)p1->x[1]);
    posTemp[i*perTet + c++] = ((float)p1->x[2]);

    posTemp[i*perTet + c++] = ((float)p3->x[0]);
    posTemp[i*perTet + c++] = ((float)p3->x[1]);
    posTemp[i*perTet + c++] = ((float)p3->x[2]);

    posTemp[i*perTet + c++] = ((float)p4->x[0]);
    posTemp[i*perTet + c++] = ((float)p4->x[1]);
    posTemp[i*perTet + c++] = ((float)p4->x[2]);

    // p3 p2 p4
    posTemp[i*perTet + c++] = ((float)p3->x[0]);
    posTemp[i*perTet + c++] = ((float)p3->x[1]);
    posTemp[i*perTet + c++] = ((float)p3->x[2]);

    posTemp[i*perTet + c++] = ((float)p2->x[0]);
    posTemp[i*perTet + c++] = ((float)p2->x[1]);
    posTemp[i*perTet + c++] = ((float)p2->x[2]);

    posTemp[i*perTet + c++] = ((float)p4->x[0]);
    posTemp[i*perTet + c++] = ((float)p4->x[1]);
    posTemp[i*perTet + c++] = ((float)p4->x[2]);
  }
  return posTemp.data();
}

float* ParticleSystem::GetStrainAllTriColors(int* size, double strainSize) {
  *size = tets.size()* 4 * 3 * 3;
  int perTet = 4 * 3 * 3;
  colorTemp.resize(*size);
  for (int i = 0; i < tets.size(); i++) {
    Particle *p1,*p2,*p3,*p4;
    GetTetP(i, p1, p2, p3, p4);

    Eigen::Matrix3d temp;
    temp << p2->x - p1->x, p3->x - p1->x, p4->x - p1->x;
    Eigen::Matrix3d deformGradient = (temp * tets[i].inversePos) - Eigen::Matrix3d::Identity();
    Eigen::Matrix3d greenStrainTensor;
    if (corotational) {
      greenStrainTensor = .5 * (deformGradient + deformGradient.transpose() +
                                                deformGradient.transpose() * deformGradient);
    } else {
      greenStrainTensor = .5 * (deformGradient + deformGradient.transpose());
    }
    double v = .4;
    Eigen::VectorXd strainVec(6);
    strainVec << greenStrainTensor(0,0), greenStrainTensor(1,1),
                                    greenStrainTensor(2,2), greenStrainTensor(1,0),
                                    greenStrainTensor(1,2), greenStrainTensor(2,0);

    Eigen::MatrixXd strainToStress(6,6);
    strainToStress << 1 - v, v, v, 0, 0, 0,
                                           v, 1 - v, v, 0, 0, 0,
                                           v, v, 1 - v, 0, 0, 0,
                                           0, 0, 0, 1 - 2*v, 0, 0,
                                           0, 0, 0, 0, 1 - 2*v, 0,
                                           0, 0, 0, 0, 0, 1 - 2*v;
    Eigen::VectorXd stressVec(6);
    stressVec = (tets[i].k/((1 + v) * (1 - 2*v))) * strainToStress * strainVec;
    Eigen::Matrix3d stressTensor;
    stressTensor << stressVec[0], stressVec[3], stressVec[5],
                    stressVec[3], stressVec[1], stressVec[4],
                    stressVec[5], stressVec[4], stressVec[2];
    double strain = stressTensor.norm();
    tets[i].strain = strain;
  }
  for (int i = 0; i < tets.size(); i++) {
    double strain = tets[i].strain;
    int c = 0;
    while(c < perTet) {
      LerpColors(strain * strainSize, &(colorTemp[i*perTet+c]));
      c += 3;
    }
  }
  return colorTemp.data();
}

void ParticleSystem::GetCameraPosAndSize(double* x, double*y, double* z) {
  *x = 0;
  *y = 0;
  *z = 0;
  for (int i = 0; i < particles.size(); ++i) {
    *x += particles[i].x[0];
    *y += particles[i].x[1];
    *z += particles[i].x[2];
  }
  for (int i = 0; i < fixed_points.size(); ++i) {
    *x += fixed_points[i].x[0];
    *y += fixed_points[i].x[1];
    *z += fixed_points[i].x[2];
  }
  *x /= particles.size() + fixed_points.size();
  *y /= particles.size() + fixed_points.size();
  *z /= particles.size() + fixed_points.size();
}

void ParticleSystem::SetupSingleSpring() {
  Reset();
  particles.emplace_back();
  particles.emplace_back();
  particles.emplace_back();
  //particles.emplace_back();
  particles[0].x << 0, 0, 0;
  particles[0].v << 0, 0.0, 0;
  particles[0].iMass = 1;
  particles[1].x << 1, .5, -1;
  particles[1].v << 0, 0.0, 0.0;
  particles[1].iMass = 1;
  particles[2].x << -1, .5, -1;
  particles[2].v << 0.0, 0.0, 0.0;
  particles[2].iMass = 1;

  //particles[3].x << 0, -.5, -1;
  //particles[3].v << 0.0, 0.0, 0.0;
  //particles[3].iMass = 1;

  CopyIntoStartPos();
  fixed_points.emplace_back();
  fixed_points[0].x << 0, -.5, -1;

  AddTet(0, 1, 2, -1);
  //ground = false;

}

void ParticleSystem::CopyIntoStartPos() {
  startPos.clear();
  for(int i = 0; i < particles.size(); ++i) {
    startPos.emplace_back();
    startPos[i] = particles[i].x;
  }
}

void ParticleSystem::SetupBendingBar() {
  Reset();
  int psize;
  double* points;
  std::vector<int> tets;
  MeshGen::GenerateBar(points, psize, tets, faces, facetotet);

  printf("Psize: %d, esize %d\n",psize, tets.size());

  for (int i = 0; i < psize; ++i) {
    particles.emplace_back();
    particles[i].x << points[i*3], points[i*3 + 1], points[i*3 + 2];
    particles[i].v << 0, 0, 0;
    particles[i].iMass = psize/20.0;
  }
  for (int i = 0; i < psize; ++i) {
    if (particles[i].x[2] == 0) {
      printf("fixed_point!\n");
      MakeFixedPoint(i, tets, faces);
      psize -= 1;
      i--;
    }
  }

  for (int i = 0; i < (tets.size()/4); ++i) {
    AddTet(tets[i*4], tets[i*4+1], tets[i*4 + 2], tets[i*4 + 3]);
  }
  //for (int i = 0; i < particles.size(); ++i) {
  //  CalculateParticleMass(i, 200.0/edges.size());
  //}
  CopyIntoStartPos(); 
  CreateOutsidePointListFromFaces();
  for (int i = 0; i < particles.size(); ++i) {
    //if (particles[i].x[2] < -2)
    //particles[i].v[1] += 5;
  }
  
  for (int i = 0; i < particles.size(); ++i) {
   // if (particles[i].x[2] >-2)
   // particles[i].v[1] += -5;
  }
  //ground = false;
  delete[] points;
}

void ParticleSystem::SetupArmadillo() {
  Reset();
  int psize;
  double* points;
  std::vector<int> tets;
  MeshGen::GenerateMesh(points, psize, tets, faces, facetotet, "Armadillo_simple2.ply");

  printf("Psize: %d, esize %d\n",psize, tets.size());

  for (int i = 0; i < psize; ++i) {
    particles.emplace_back();
    particles[i].x << points[i*3], points[i*3 + 1], points[i*3 + 2];
    particles[i].v << 0, 0, 0;
    particles[i].iMass = psize/20.0;
  }
  for (int i = 0; i < psize; ++i) {
    if (particles[i].x[1] < -6 && particles[i].x[0] < 2) {
      printf("fixed_point!\n");
      MakeFixedPoint(i, tets, faces);
      psize -= 1;
      i--;
    }
  }

  for (int i = 0; i < (tets.size()/4); ++i) {
    AddTet(tets[i*4], tets[i*4+1], tets[i*4 + 2], tets[i*4 + 3]);
  }
  //for (int i = 0; i < particles.size(); ++i) {
  //  CalculateParticleMass(i, 200.0/edges.size());
  //}
  CopyIntoStartPos(); 
  CreateOutsidePointListFromFaces();
  //for (int i = 0; i < particles.size(); ++i) {
  //  if (particles[i].x[2] < -6)
  //  particles[i].v[1] += .5;
  //}
  
  for (int i = 0; i < particles.size(); ++i) {
   // if (particles[i].x[2] >-2)
   // particles[i].v[1] += -5;
  }
  //ground = true;
  delete[] points;
  printf("Number of faces%d\n", faces.size()/3);
}

void ParticleSystem::SetupMeshFile(const char* filename) {
  Reset();
  int psize;
  double* points;
  std::vector<int> tets;
  MeshGen::GenerateMesh(points, psize, tets, faces, facetotet, filename);
  if (points == NULL) {
    return;
  }

  printf("Psize: %d, Number of tets %d\n",psize, tets.size());

  double lowestpoint = -10;
  for (int i = 0; i < psize; ++i) {
    particles.emplace_back();
    particles[i].x << points[i*3], points[i*3 + 1], points[i*3 + 2];
    particles[i].v << 0, 0, 0;
    particles[i].iMass = psize/20.0;
    if (points[i*3 + 1] > lowestpoint) {
      lowestpoint = points[i*3+1];
    }
  }

  //Make lowest points fixed
  //for (int i = 0; i < psize; ++i) {
  //  if (particles[i].x[1] > lowestpoint - .1) {
  //    printf("fixed_point!\n");
  //    MakeFixedPoint(i, tets, faces);
  //    psize -= 1;
  //    i--;
  //  }
  //}

  for (int i = 0; i < (tets.size()/4); ++i) {
    AddTet(tets[i*4], tets[i*4+1], tets[i*4 + 2], tets[i*4 + 3]);
  }
  Particle *g1,* g2,* g3, * g4;
  g1 = new Particle();
  g2 = new Particle();
  g3 = new Particle();
  g4 = new Particle();
  g1->x << 15, lowestpoint + 1, -10;
  g1->v << 0, 0, 0;
  g2->x << 0, lowestpoint + 1, 30;
  g2->v << 0, 0, 0;
  g3->x << -15, lowestpoint + 1, -10;
  g3->v << 0, 0,0;
  g4->x << 0, lowestpoint + 3, 0;
  g4->v << 0,0,0;
  fixed_points.emplace_back(*g1);
  fixed_points.emplace_back(*g2);
  fixed_points.emplace_back(*g3);
  fixed_points.emplace_back(*g4);
  outsidePoints.push_back(-1 * fixed_points.size());
  outsidePoints.push_back(-1 * fixed_points.size() + 1);
  outsidePoints.push_back(-1 * fixed_points.size() + 2);
  outsidePoints.push_back(-1 * fixed_points.size() + 3);

  // top tri
  faces.push_back(-1 * fixed_points.size() + 3);
  faces.push_back(-1 * fixed_points.size() + 2);
  faces.push_back(-1 * fixed_points.size() + 1);
  
  faces.push_back(-1 * fixed_points.size() + 3);
  faces.push_back(-1 * fixed_points.size() + 2);
  faces.push_back(-1 * fixed_points.size());

  faces.push_back(-1 * fixed_points.size() + 2);
  faces.push_back(-1 * fixed_points.size() + 1);
  faces.push_back(-1 * fixed_points.size());
  
  faces.push_back(-1 * fixed_points.size() + 1);
  faces.push_back(-1 * fixed_points.size() + 3);
  faces.push_back(-1 * fixed_points.size());
  CopyIntoStartPos();
  CreateOutsidePointListFromFaces();
  groundLevel = lowestpoint + 1;
  delete[] points;
  printf("Number of faces%d\n", faces.size()/3);
}

void ParticleSystem::CreateOutsidePointListFromFaces() {
  useColSys = true;
  outsidePoints.clear();
  faceToOut.clear();
  for (int i = 0; i < faces.size(); ++i) {
    faceToOut.push_back(0);
    bool addPoint = true;
    //printf("loop %d\n", i);
    for(int j = outsidePoints.size() - 1; j >= 0 && addPoint; --j) {
      if (outsidePoints[j] == faces[i]) {
        faceToOut[faceToOut.size() - 1] = j;
        addPoint = false;
      }
    }
    if (addPoint) {
      outsidePoints.push_back(faces[i]);
      faceToOut[faceToOut.size() - 1] = outsidePoints.size() - 1;
    }
  }
  std::vector<Eigen::Vector3d> verts;
  for (int i = 0; i < outsidePoints.size(); ++i) {
    Particle* x;
    //printf("Getting point %i\n", outsidePoints[i]);
    GetPointP(outsidePoints[i], x);
    verts.push_back(x->x);
  }
  //printf("starting init System\n");
  colSys->InitSystem(verts, faceToOut);
}

void ParticleSystem::MakeFixedPoint(int p, std::vector<int>& edges, std::vector<int>& faces) {
  fixed_points.emplace_back();
  fixed_points[fixed_points.size() - 1].x = particles[p].x;
  for (int i = 0; i < edges.size(); ++i) {
    if (edges[i] == p) {
      edges[i] = -1*fixed_points.size();
    } else if (edges[i] > p) {
      edges[i] -= 1;
    }
  }
  for (int i = 0; i < faces.size(); ++i) {
    if (faces[i] == p) {
      faces[i] = -1*fixed_points.size();
    } else if (faces[i] > p) {
      faces[i] -= 1;
    }
  }
  particles.erase(particles.begin() + p);
}

void ParticleSystem::CalculateParticleMass(int i, float springMass) {
 /* float mass = 0;
  for (int j = 0; j < springs.size(); ++j) {
    if (springs[j].to == i) mass += springMass/2;
    if (springs[j].from == i) mass += springMass/2;
  }
  if (mass == 0) mass = 1;
  particles[i].iMass = 1/mass;*/
}


void ParticleSystem::SetSpringProperties(double k, double vol, double c, double grav, double gStiffness, double mStiffness) {
  stiffness = k;
  volConserve = vol;
  dampness = c;
  gravity = grav;
  groundStiffness = gStiffness;
  mouseStiffness = mStiffness;
}

void ParticleSystem::ComputeForces() {}
void ParticleSystem::ExplicitEuler(double timestep) {
  /*phaseTemp.resize(particles.size() * 6);
  for (int i = 0; i < particles.size(); i++) {
    phaseTemp[i * 6] = particles[i].x[0];
    phaseTemp[i * 6 + 1] = particles[i].x[1];
    phaseTemp[i * 6 + 2] = particles[i].x[2];
    phaseTemp[i * 6 + 3] = particles[i].v[0];
    phaseTemp[i * 6 + 4] = particles[i].v[1];
    phaseTemp[i * 6 + 5] = particles[i].v[2];
  }*/
  ComputeForces();
  for (int i = 0; i < particles.size(); i++) {
    particles[i].v += particles[i].f * particles[i].iMass * timestep;
    particles[i].x += particles[i].v * timestep;
  }
  /*
  ComputeForces();
  for (int i = 0; i < particles.size(); i++) {
    particles[i].x[0] = phaseTemp[i * 6];
    particles[i].x[1] = phaseTemp[i * 6 + 1];
    particles[i].x[2] = phaseTemp[i * 6 + 2];
    particles[i].v[0] = phaseTemp[i * 6 + 3];
    particles[i].v[1] = phaseTemp[i * 6 + 4];
    particles[i].v[2] = phaseTemp[i * 6 + 5];
    particles[i].v += particles[i].f * particles[i].iMass * timestep;
    //particles[i].v *= .95;
    particles[i].x += particles[i].v * timestep;
  }*/
}


void ParticleSystem::AddTet(int x1, int x2, int x3, int x4) {
  tets.emplace_back();
  int index = tets.size() - 1;
  tets[index].to[0] = x1;
  tets[index].to[1] = x2;
  tets[index].to[2] = x3;
  tets[index].to[3] = x4;

  tets[index].k = stiffness;
  tets[index].c = dampness;

  Particle *p1,*p2,*p3,*p4;
  GetTetP(index, p1, p2, p3, p4);
  Eigen::Matrix3d temp;
  temp << p2->x - p1->x, p3->x - p1->x, p4->x - p1->x;

  tets[index].oldPos[0] = p2->x - p1->x;
  tets[index].oldPos[1] = p3->x - p1->x;
  tets[index].oldPos[2] = p4->x - p1->x;
  tets[index].posDet = temp.determinant();
  tets[index].inversePos = temp.inverse();
}

void ParticleSystem::GetTetP(int i, Particle*& x1, Particle*& x2, Particle*& x3, Particle*& x4) {
  if (tets[i].to[0] < 0)
    x1 = &(fixed_points[tets[i].to[0] * -1 - 1]);
  else
    x1 = &(particles[tets[i].to[0]]);

  if (tets[i].to[1] < 0)
    x2 = &(fixed_points[tets[i].to[1] * -1 - 1]);
  else
    x2 = &(particles[tets[i].to[1]]);

  if (tets[i].to[2] < 0)
    x3 = &(fixed_points[tets[i].to[2] * -1 - 1]);
  else
    x3 = &(particles[tets[i].to[2]]);

  if (tets[i].to[3] < 0)
    x4 = &(fixed_points[tets[i].to[3] * -1 - 1]);
  else
    x4 = &(particles[tets[i].to[3]]);
}

void ParticleSystem::GetPointP(int i, Particle*& x1) {
  if (i < 0)
    x1 = &(fixed_points[i * -1 - 1]);
  else
    x1 = &(particles[i]);
}

