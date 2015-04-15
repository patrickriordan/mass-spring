#include "particle_system.h"
#include "draw_delegate.h"
#include "duckrace.h"
#include "Eigen/Sparse"
#include "Eigen/IterativeLinearSolvers"

ParticleSystem::ParticleSystem() {
  mouseP = -1;
  stiffness = 100;
  dampness = 10;
  gravity = 9.8;
  ground = false;
}

void ParticleSystem::Update(double timestep, bool implicit) {
  if (implicit) ImplicitEuler(timestep);
  else ExplicitEuler(timestep);

  if (ground) {
    for (int i = 0; i < particles.size(); i++) {
      if (particles[i].x[1] > DDHEIGHT-20) {
        particles[i].x[1] = DDHEIGHT - 20;
        if (particles[i].v[1] > 0) {
            particles[i].v[1] = -.8 * particles[i].v[1];
          /*if (particles[i].v[0] > 0) {
            particles[i].v[0] -= 5000 * timestep;
          } else {
            particles[i].v[0] += 5000 * timestep;
          }*/
        }
      }
    }
  }
}

float* ParticleSystem::GetPositions2d(int* size, double x, double y, double zoom) {
  *size = springs.size()*4 + 4;
  posTemp.resize(*size);
  for (int i = 0; i < springs.size(); i++) {
    Particle* to,*from;
    GetSpringP(i, to, from);

    posTemp[i*4] = ((float)to->x[0] - x) * zoom;
    posTemp[i*4 + 1] = ((float)to->x[1] - y) * zoom;
    posTemp[i*4 + 2] = ((float)from->x[0] - x) * zoom;
    posTemp[i*4 + 3] = ((float)from->x[1] - y) * zoom;
  }
  posTemp[*size - 4] = 0;
  posTemp[*size - 3] = (10 - y) * zoom;
  posTemp[*size - 2] = DDWIDTH;
  posTemp[*size - 1] = (10 - y) * zoom;
  return posTemp.data();
}

void ParticleSystem::GetCameraPosAndSize(double* x, double*y, double* zoom) {
  *x = 0;
  *y = 0;
  for (int i = 0; i < particles.size(); ++i) {
    *x += particles[i].x[0];
    *y += particles[i].x[1];
  }
  for (int i = 0; i < fixed_points.size(); ++i) {
    *x += fixed_points[i].x[0];
    *y += fixed_points[i].x[1];
  }
  *x /= particles.size() + fixed_points.size();
  *y /= particles.size() + fixed_points.size();
  double out;
  out = .01;
  for (int i = 0; i < particles.size(); ++i) {
    if (particles[i].x[0] - *x > out)
      out = particles[i].x[0] - *x;
    if (*x - particles[i].x[0] > out)
      out = *x - particles[i].x[0];
    if (particles[i].x[1] - *y > out)
      out = particles[i].x[1] - *y;
    if (*y - particles[i].x[1] > out)
      out = *y - particles[i].x[1];
  }
  for (int i = 0; i < fixed_points.size(); ++i) {
    if (i == mouseP) continue;
    if (fixed_points[i].x[0] - *x > out)
      out = fixed_points[i].x[0] - *x;
    if (*x - fixed_points[i].x[0] > out)
      out = *x - fixed_points[i].x[0];
    if (fixed_points[i].x[1] - *y > out)
      out = fixed_points[i].x[1] - *y;
    if (*y - fixed_points[i].x[1] > out)
      out = *y - fixed_points[i].x[1];
  }
  *zoom = (DDWIDTH/2 - 100) / out;
  if (*zoom > 1000) *zoom = 1000;
  if (*zoom < 0.001) *zoom = 0.001;
  *x -= (DDWIDTH/2) / (*zoom);
  *y -= (DDHEIGHT/2) / (*zoom);
}

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

float* ParticleSystem::GetColors(int* size, int strainSize) {
  *size = springs.size()*6 + 6;
  colorTemp.resize(*size);
  for (int i = 0; i < springs.size(); i++) {
    Particle* to,*from;
    GetSpringP(i, to, from);

    float strain = ((to->x - from->x).norm() - springs[i].L) / springs[i].L;
    if (strain < 0) strain *= -1;

    strain *= strainSize;//10 + springs[i].k/1000;
    LerpColors(strain, &(colorTemp[i*6]));
    /*colorTemp[i*6] = strain - 1;
    colorTemp[i*6 + 1] = 0;
    colorTemp[i*6 + 2] = strain;
*/
    LerpColors(strain, &(colorTemp[i*6+3]));
/*    colorTemp[i*6 + 3] = strain - 1;
    colorTemp[i*6 + 4] = 0;
    colorTemp[i*6 + 5] = strain;
*/
  }
  colorTemp[*size - 6] = 0.0;
  colorTemp[*size - 5] = 0.0;
  colorTemp[*size - 4] = 0.0;
  colorTemp[*size - 3] = 0.0;
  colorTemp[*size - 2] = 0.0;
  colorTemp[*size - 1] = 0.0;
  return colorTemp.data();
}

void ParticleSystem::SetupSingleSpring() {
  Reset();
  particles.emplace_back();
  particles.emplace_back();
  particles[0].x << 0.0, 0.0;
  particles[0].v << -1, 0.0;
  particles[0].iMass = 1;
  particles[1].x << 0.0, 0.1;
  particles[1].v << 1.0, 0.0;
  particles[1].iMass = 1;

  springs.emplace_back();
  springs[0].to = 0;
  springs[0].from = 1;
  springs[0].k = stiffness;
  springs[0].L = .1;
  springs[0].c = dampness;
  gravity = 0;
  ground = false;

}

void ParticleSystem::SetupTriangle() {
  Reset();
  particles.emplace_back();
  particles.emplace_back();
  particles.emplace_back();
  particles[0].x << 0.0, 0.0;
  particles[0].v << 0.0, 0.0;
  particles[0].iMass = 1;
  particles[1].x << 1.0, 0.0;
  particles[1].v << 0.0, 0.0;
  particles[1].iMass = 1;
  particles[2].x << 1.0, 1.0;
  particles[2].v << 0.0, 0.0;
  particles[2].iMass = 1;

  AddSpring(0, 1);
  AddSpring(0, 2);
  AddSpring(1, 2);
  gravity = 0;
  ground = true;
}

// Assumes this is the first Setup function called
void ParticleSystem::SetupTriforce() {
  Reset();
  // 6 particles
  particles.emplace_back();
  particles.emplace_back();
  particles.emplace_back();
  particles.emplace_back();
  particles.emplace_back();
  particles.emplace_back();
  particles[0].x << 20.0, 10.0;
  particles[0].v << 0.0, 0.0;
  particles[0].iMass = 1;
  particles[1].x << 17.5, 12.5;
  particles[1].v << 0.0, 0.0;
  particles[1].iMass = 1;
  particles[2].x << 22.5, 12.5;
  particles[2].v << 0.0, 0.0;
  particles[2].iMass = 1;

  particles[3].x << 15.0, 15.0;
  particles[3].v << 0.0, 0.0;
  particles[3].iMass = 1;
  particles[4].x << 20.0, 15.0;
  particles[4].v << 0.0, 0.0;
  particles[4].iMass = 1;
  particles[5].x << 25.0, 15.0;
  particles[5].v << 0.0, 0.0;
  particles[5].iMass = 1;

  // 9 springs
  springs.emplace_back();
  springs.emplace_back();
  springs.emplace_back();

  springs.emplace_back();
  springs.emplace_back();
  springs.emplace_back();
  springs.emplace_back();
  springs.emplace_back();
  springs.emplace_back();

  springs[0].to = 0;
  springs[0].from = 1;
  springs[0].k = stiffness;
  springs[0].L = 5;
  springs[0].c = dampness;

  springs[1].to = 0;
  springs[1].from = 2;
  springs[1].k = stiffness;
  springs[1].L = 5;
  springs[1].c = dampness;

  springs[2].to = 1;
  springs[2].from = 2;
  springs[2].k = stiffness;
  springs[2].L = 5;
  springs[2].c = dampness;

  springs[3].to = 1;
  springs[3].from = 3;
  springs[3].k = stiffness;
  springs[3].L = 5;
  springs[3].c = dampness;
  springs[4].to = 1;
  springs[4].from = 4;
  springs[4].k = stiffness;
  springs[4].L = 5;
  springs[4].c = dampness;
  springs[5].to = 2;
  springs[5].from = 4;
  springs[5].k = stiffness;
  springs[5].L = 5;
  springs[5].c = dampness;
  springs[6].to = 2;
  springs[6].from = 5;
  springs[6].k = stiffness;
  springs[6].L = 5;
  springs[6].c = dampness;
  springs[7].to = 3;
  springs[7].from = 4;
  springs[7].k = stiffness;
  springs[7].L = 5;
  springs[7].c = dampness;
  springs[8].to = 4;
  springs[8].from = 5;
  springs[8].k = stiffness;
  springs[8].L = 5;
  springs[8].c = dampness;

  fixed_points.emplace_back();
  fixed_points[0].x << 0, 0;
  AddSpring(5, -1);
  gravity = 9.8;
  ground = false;
}

void ParticleSystem::SetupBall(double x, double y) {
  Duckrace::MakeBall(this, x, y);
}

void ParticleSystem::SetupMouseSpring(int to) {
  if (mouseP == -1) {
    fixed_points.emplace_back();
    mouseP = fixed_points.size() - 1;
    fixed_points[mouseP].x << 0.5, 0.5;
    fixed_points[mouseP].v << 0, 0;
    fixed_points[mouseP].iMass = 1;
  }
  springs.emplace_back();
  //AddSpring(to, -1 * mouseP - 1);
  mouseSprings.push_back(springs.size() - 1);
  Spring* tempS = &(springs[springs.size() - 1]);
  tempS->to = to;
  tempS->from = -1 * mouseP - 1;
  tempS->L = 6;
  tempS->k = stiffness;
  tempS->c = dampness;
}

void ParticleSystem::SetMouseSpring(bool enabled) {
  for (int i = 0; i < mouseSprings.size(); ++i) {
    if (enabled) {
      springs[mouseSprings[i]].k = stiffness;
      springs[mouseSprings[i]].c = dampness;
    } else {
      springs[mouseSprings[i]].k = 0;
      springs[mouseSprings[i]].c = 0;
    }
  }
}


void ParticleSystem::SetMousePos(double x, double y) {
   if (mouseP != -1) {
     fixed_points[mouseP].x << x, y;
     fixed_points[mouseP].v << 0, 0;
   }
}

void ParticleSystem::SetupBridge2(int bridgeL) {
  Reset();
  fixed_points.emplace_back();
  fixed_points.emplace_back();

  //int bridgeL = 10;
  fixed_points[0].x << -4, 0;
  fixed_points[1].x << bridgeL*4, 0;
  for (int i = 0; i < bridgeL; ++i) {
    particles.emplace_back();
    particles[i].x << i* 4, 0;
    particles[i].v << 0, 0;
    particles[i].iMass = 1;
  }
  int l2start = bridgeL;
  for (int i = bridgeL; i < bridgeL*2 +1; i++) {
    particles.emplace_back();
    particles[i].x << (i - bridgeL)* 4 - 2, -4;
    particles[i].v << 0, 0;
    particles[i].iMass = 1;
  }

  AddSpring(0, -1);
  AddSpring(bridgeL - 1, -2);
  AddSpring(l2start, -1);
  AddSpring(l2start + bridgeL, -2);

  for (int i = 0; i < bridgeL - 1; ++i) {
    AddSpring(i, i+1);
  }
  for (int i = l2start; i < bridgeL*2; ++i) {
    AddSpring(i, i+1);
  }
  for (int i = 0; i < bridgeL; ++i) {
    AddSpring(l2start + i, i);
    AddSpring(i, l2start + i + 1);
  }
  gravity = 9.8;
  ground = false;
  /*
  for (int i = 0; i < bridgeL - 1; i++) {
    AddSpring(i*2 + ((i+1)%2), (i+1)*2 + (i%2));
  }

  for (int i = 0; i < bridgeL - 1; i++) {
    AddSpring(i*2, (i+1)*2);
    AddSpring(i*2 + 1, (i+1)*2 + 1);
  }*/
}
void ParticleSystem::SetupBridge() {
  Reset();
  fixed_points.emplace_back();
  fixed_points.emplace_back();
  fixed_points.emplace_back();
  fixed_points.emplace_back();

  fixed_points[0].x << 50, 300;
  fixed_points[1].x << 50, 350;
  fixed_points[2].x << DDWIDTH - 50, 300;
  fixed_points[3].x << DDWIDTH - 50, 350;
  int last;
  for (int i = 0; i < (DDWIDTH-150)/50; i++) {
    particles.emplace_back();
    particles[i*2].x << 100 + i * 50, 300;
    particles[i*2].v << 0, 0;
    particles[i*2].iMass = 1;
    particles.emplace_back();
    particles[i*2+1].x << 100 + i * 50, 350;
    particles[i*2+1].v << 0, 0;
    particles[i*2+1].iMass = 1;
    last = i*2;
  }
  springs.emplace_back();
  springs.emplace_back();
  springs.emplace_back();
  springs.emplace_back();
  springs[0].to = 0;
  springs[0].from = -1;
  springs[0].L = 50;
  springs[0].k = stiffness;
  springs[0].c = dampness;

  springs[1].to = 1;
  springs[1].from = -2;
  springs[1].L = 50;
  springs[1].k = stiffness;
  springs[1].c = dampness;

  springs[2].to = last;
  springs[2].from = -3;
  springs[2].L = 50;
  springs[2].k = stiffness;
  springs[2].c = dampness;

  springs[3].to = last + 1;
  springs[3].from = -4;
  springs[3].L = 50;
  springs[3].k = stiffness;
  springs[3].c = dampness;

  int curS = springs.size() - 1;
  for (int i = 0; i < (DDWIDTH-150)/50 - 1; i++) {
    springs.emplace_back();
    curS++;
    springs[curS].to = i*2+ (i%2);
    springs[curS].from = (i+1)*2 + ((i+1)%2);
    springs[curS].L = 70.71;
    springs[curS].k = stiffness/10;
    springs[curS].c = dampness/10;
    springs.emplace_back();
    curS++;
    springs[curS].to = i*2+ ((i+1)%2);
    springs[curS].from = (i+1)*2 + ((i)%2);
    springs[curS].L = 70.71;
    springs[curS].k = stiffness/10;
    springs[curS].c = dampness/10;
  }

  for (int i = 0; i < (DDWIDTH-150)/50 - 1; i++) {
    springs.emplace_back();
    curS++;
    springs[curS].to = i*2;
    springs[curS].from = (i+1)*2;
    springs[curS].L = 50;
    springs[curS].k = stiffness;
    springs[curS].c = dampness;
    springs.emplace_back();
    curS++;
    springs[curS].to = i*2 + 1;
    springs[curS].from = (i+1)*2 + 1;
    springs[curS].L = 50;
    springs[curS].k = stiffness;
    springs[curS].c = dampness;
  }
  gravity = 9.8;
  ground = false;
}

void ParticleSystem::Reset() {
  mouseP = -1;
  particles.clear();
  fixed_points.clear();
  springs.clear();
  mouseSprings.clear();
}

void ParticleSystem::SetSpringProperties(double k, double c) {
  stiffness = k;
  dampness = c;
}

void ParticleSystem::ComputeForces() {
  //Zero all forces
  for (int i = 0; i < particles.size(); i++) {
    particles[i].f << 0.0, gravity/particles[i].iMass;
  }
  //Compute spring forces
  int sSize = springs.size();
  for (int i = 0; i < sSize; i++) {
    Particle *to, *from;
    GetSpringP(i, to, from);

    Eigen::Vector2d springdir;
    double length = (to->x - from->x).norm();
    if (length != 0) {
      springdir = (to->x - from->x) / length;
    } else {
      springdir << 1, 0;
      length = Eigen::NumTraits<double>::epsilon();
    }
    Eigen::Vector2d force =  (length - springs[i].L) * springs[i].k * springdir;
    // Damping
    force += springs[i].c * springdir * ((to->v - from->v).dot(springdir));
    to->f -= force;
    from->f += force;
  }
}

void ParticleSystem::ExplicitEuler(double timestep) {
  phaseTemp.resize(particles.size() * 4);
  for (int i = 0; i < particles.size(); i++) {
    phaseTemp[i * 4] = particles[i].x[0];
    phaseTemp[i * 4 + 1] = particles[i].x[1];
    phaseTemp[i * 4 + 2] = particles[i].v[0];
    phaseTemp[i * 4 + 3] = particles[i].v[1];
  }
  ComputeForces();
  for (int i = 0; i < particles.size(); i++) {
    particles[i].v += particles[i].f * particles[i].iMass * timestep/2;
    particles[i].x += particles[i].v * timestep/2;
  }
  ComputeForces();
  for (int i = 0; i < particles.size(); i++) {
    particles[i].x[0] = phaseTemp[i * 4];
    particles[i].x[1] = phaseTemp[i * 4 + 1];
    particles[i].v[0] = phaseTemp[i * 4 + 2];
    particles[i].v[1] = phaseTemp[i * 4 + 3];
    particles[i].v += particles[i].f * particles[i].iMass * timestep;
    particles[i].x += particles[i].v * timestep;
  }
}

void ParticleSystem::ImplicitEuler(double timestep) {
  int vSize = 2 * particles.size();
  Eigen::MatrixXd A(vSize, vSize);
  Eigen::VectorXd b(vSize);

  Eigen::MatrixXd dfdx(vSize, vSize);
  Eigen::MatrixXd dfdv(vSize, vSize);
  dfdx.setZero();
  dfdv.setZero();
  Eigen::Matrix2d temp;
  Eigen::Matrix2d tempdv;
  for (int i = 0; i < springs.size(); i++) {
    Particle *to, *from;
    if (springs[i].to < 0)
      to = &(fixed_points[springs[i].to * -1 - 1]);
    else
      to = &(particles[springs[i].to]);
    if (springs[i].from < 0)
      from = &(fixed_points[springs[i].from * -1 - 1]);
    else
      from = &(particles[springs[i].from]);
    Eigen::Vector2d springdir = from->x - to->x;
    double length = springdir.norm();
    if (length == 0)  {
      //printf("zero %d\n", i);
      continue;
    }
    // Jacobian for Hookean spring force
    //temp = ( (springdir * springdir.transpose())/(springdir.transpose() * springdir) + ( Eigen::MatrixXd::Identity(3,3) - (springdir * springdir.transpose())/(springdir.transpose() * springdir)) * ( 1- springs[i].L/length)) * springs[i].k;
    temp = springs[i].k * ( (1 - springs[i].L/length) * (Eigen::MatrixXd::Identity(2,2) - ((springdir/length) * (springdir/length).transpose()))
           + ((springdir/length) * (springdir/length).transpose()));

    tempdv = springs[i].c * ((springdir/length) * (springdir/length).transpose());

    if (springs[i].to >= 0 && springs[i].from >= 0) {
      dfdx.block<2,2>(springs[i].to * 2, springs[i].from * 2) += temp;
      dfdx.block<2,2>(springs[i].from * 2, springs[i].to * 2) += temp;

      dfdv.block<2,2>(springs[i].to * 2, springs[i].from * 2) += tempdv;
      dfdv.block<2,2>(springs[i].from * 2, springs[i].to * 2) += tempdv;
    }
    if (springs[i].to >= 0) {
      dfdx.block<2,2>(springs[i].to * 2, springs[i].to * 2) -= temp;
      dfdv.block<2,2>(springs[i].to * 2, springs[i].to * 2) -= tempdv;
    }
    if (springs[i].from >= 0) {
      dfdx.block<2,2>(springs[i].from * 2, springs[i].from * 2) -= temp;
      dfdv.block<2,2>(springs[i].from * 2, springs[i].from * 2) -= tempdv;
    }
  }
  ComputeForces();
  Eigen::VectorXd v_0(vSize);
  Eigen::VectorXd f_0(vSize);
  A.setZero();
  for (int i = 0; i < particles.size(); i++) {
    v_0[i * 2] = particles[i].v[0];
    v_0[i * 2 + 1] = particles[i].v[1];
    f_0[i * 2] = particles[i].f[0];
    f_0[i * 2 + 1] = particles[i].f[1];
    A.coeffRef(i*2,i*2) = 1/particles[i].iMass;
    A.coeffRef(i*2+1,i*2+1) = 1/particles[i].iMass;
  }
  b = timestep * (f_0 + timestep * (dfdx * v_0));
  A = A - (timestep * dfdv + timestep * timestep * dfdx);
  Eigen::VectorXd vdiff(vSize);
  Eigen::ConjugateGradient<Eigen::MatrixXd > cg;
  cg.compute(A);
  vdiff = cg.solve(b);
  for (int i = 0; i < particles.size(); i++) {
    particles[i].v[0] += vdiff[i * 2];
    particles[i].v[1] += vdiff[i * 2 + 1];

    particles[i].x += timestep * particles[i].v;
  }
}

void ParticleSystem::ImplicitEulerSolveForNewV(double timestep) {
  int vSize = 2 * particles.size();
  Eigen::MatrixXd A(vSize, vSize);
  Eigen::VectorXd b(vSize);

  Eigen::MatrixXd dfdx(vSize, vSize);
  Eigen::MatrixXd dfdv(vSize, vSize);
  dfdx.setZero();
  dfdv.setZero();
  Eigen::Matrix2d temp;
  Eigen::Matrix2d tempdv;
  for (int i = 0; i < springs.size(); i++) {
    Particle *to, *from;
    if (springs[i].to < 0)
      to = &(fixed_points[springs[i].to * -1 - 1]);
    else
      to = &(particles[springs[i].to]);
    if (springs[i].from < 0)
      from = &(fixed_points[springs[i].from * -1 - 1]);
    else
      from = &(particles[springs[i].from]);
    Eigen::Vector2d springdir = from->x - to->x;
    double length = springdir.norm();
    if (length == 0)  {
      //printf("zero %d\n", i);
      continue;
    }
    // Jacobian for Hookean spring force
    temp = (springs[i].k / (length*length)) * (((length - springs[i].L)/length) * (springdir.dot(springdir)) * Eigen::MatrixXd::Identity(2,2) + (1 - (length - springs[i].L)/length) * springdir * springdir.transpose());
    //temp = springs[i].k * ( (1 - springs[i].L/length) * (Eigen::MatrixXd::Identity(2,2) - ((springdir/length) * (springdir/length).transpose()))
    //       + ((springdir/length) * (springdir/length).transpose()));

    tempdv = springs[i].c * ((springdir/length) * (springdir/length).transpose());

    if (springs[i].to >= 0 && springs[i].from >= 0) {
      dfdx.block<2,2>(springs[i].to * 2, springs[i].from * 2) += temp;
      dfdx.block<2,2>(springs[i].from * 2, springs[i].to * 2) += temp;

      dfdv.block<2,2>(springs[i].to * 2, springs[i].from * 2) += tempdv;
      dfdv.block<2,2>(springs[i].from * 2, springs[i].to * 2) += tempdv;
    }
    if (springs[i].to >= 0) {
      dfdx.block<2,2>(springs[i].to * 2, springs[i].to * 2) -= temp;
      dfdv.block<2,2>(springs[i].to * 2, springs[i].to * 2) -= tempdv;
    }
    if (springs[i].from >= 0) {
      dfdx.block<2,2>(springs[i].from * 2, springs[i].from * 2) -= temp;
      dfdv.block<2,2>(springs[i].from * 2, springs[i].from * 2) -= tempdv;
    }
  }
  ComputeForces();
  Eigen::VectorXd v_0(vSize);
  Eigen::VectorXd f_0(vSize);
  A.setZero();
  for (int i = 0; i < particles.size(); i++) {
    v_0[i * 2] = particles[i].v[0];
    v_0[i * 2 + 1] = particles[i].v[1];
    f_0[i * 2] = particles[i].f[0];
    f_0[i * 2 + 1] = particles[i].f[1];
    A.coeffRef(i*2,i*2) = 1/particles[i].iMass;
    A.coeffRef(i*2+1,i*2+1) = 1/particles[i].iMass;
  }
  b = A * v_0 + timestep * f_0;
  A = A - timestep * timestep * dfdx - timestep *dfdv;
  Eigen::VectorXd vnew(vSize);
  Eigen::ConjugateGradient<Eigen::MatrixXd > cg;
  cg.compute(A);
  vnew = cg.solve(b);
  for (int i = 0; i < particles.size(); i++) {
    particles[i].v[0] = vnew[i * 2];
    particles[i].v[1] = vnew[i * 2 + 1];

    particles[i].x += timestep * particles[i].v;
  }
}
void ParticleSystem::AddSpring(int to, int from) {
  springs.emplace_back();
  int index = springs.size() - 1;
  springs[index].to = to;
  springs[index].from = from;
  springs[index].k = stiffness;
  springs[index].c = dampness;
  Particle*s1, *s2;
  GetSpringP(index, s1, s2);
  springs[index].L = (s1->x - s2->x).norm();
}

void ParticleSystem::GetSpringP(int i, Particle*& to, Particle*& from) {
  if (springs[i].to < 0)
    to = &(fixed_points[springs[i].to * -1 - 1]);
  else
    to = &(particles[springs[i].to]);
  if (springs[i].from < 0)
    from = &(fixed_points[springs[i].from * -1 - 1]);
  else
    from = &(particles[springs[i].from]);
}
