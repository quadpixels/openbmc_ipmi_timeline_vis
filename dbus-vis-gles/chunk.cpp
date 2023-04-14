#include <string.h>

#include "chunk.hpp"
#include "util.hpp"

const int Chunk::kSize = 32;
const float Chunk::kL0 = 1.0f;
unsigned int Chunk::shader_program = 0;
bool Chunk::verbose = false;

Chunk::Chunk() {
  vbo = tri_count = 0;
  const int num_cells = kSize * kSize * kSize;
  block = new unsigned char[num_cells];
  light = new int[num_cells];
  pos = glm::vec3(-10, 0, -10);
  memset(block, 0, sizeof(char) * num_cells);
  memset(light, 0, sizeof(int) * num_cells);
  is_dirty = true;
}

Chunk::Chunk(Chunk& other) {
  is_dirty = true;
  pos = other.pos;
  tri_count = vbo = 0;
  const int num_cells = kSize * kSize * kSize;
  block = new unsigned char[num_cells];
  light = new int[num_cells];
  memcpy(block, other.block, sizeof(char)*num_cells);
  memcpy(light, other.light, sizeof(int)*num_cells);
}

void Chunk::BuildBuffers(Chunk* neighbors[26]) {
  if (vbo != (unsigned)-999) {
    glDeleteBuffers(1, &vbo);
    vbo = 0;
  }

  // axis=0  x=u y=v z=w
  // axis=1  x=w y=u z=v
  // axis=2  x=v y=w z=u

  const int x_idxs[] = { 0,2,1 }, y_idxs[] = { 1,0,2 }, z_idxs[] = { 2,1,0 };

  const int u_axes[] = { 0,1,2 }, v_axes[] = { 1,2,0 }, w_axes[] = { 2,0,1 };

  const glm::vec3 units[] = { glm::vec3(1,0,0), glm::vec3(0,1,0), glm::vec3(0,0,1) };
  const int SIZE = (kSize+1) * (kSize+1) * (kSize+1);
  float* tmp_vert = new float[SIZE * 36 * 3],
       * tmp_norm = new float[SIZE * 36 * 3];
  int  * tmp_data = new int[SIZE * 36];
  int  * tmp_ao   = new int[SIZE * 36];
  int idx_v = 0, idx_n = 0, idx_data = 0, idx_ao = 0;
  tri_count = 0;

  const float coord_min = 0;//(size-1) * l0 * 0.5f;
  for (int aidx = 0; aidx < 3; aidx++) {
    for (int w=0; w<kSize; w++) {
      for (int d=0; d<2; d++) {
        // 1. Generate Scratch
        std::vector<int> scratch(kSize * kSize, 0);
        glm::vec3 u0 = units[u_axes[aidx]], v0 = units[v_axes[aidx]], w0 = units[w_axes[aidx]];
        if (d==1) w0 = -w0;
        for (int u=0; u<kSize; u++) {
          for (int v=0; v<kSize; v++) {
            const int sidx = u * kSize + v;
            float uvw[] = { float(u), float(v), float(w) };
            glm::vec3 xyz = glm::vec3(uvw[x_idxs[aidx]], uvw[y_idxs[aidx]], uvw[z_idxs[aidx]]);
            glm::vec3 xyz_next = xyz;
            xyz_next[w_axes[aidx]] += (d==0 ? 1.0f : -1.0f);
            glm::vec3 origin(-coord_min + xyz.x*kL0, -coord_min + xyz.y*kL0, -coord_min + xyz.z*kL0);

            const int ix_xyz = IX(int(xyz.x), int(xyz.y), int(xyz.z));
            int voxel = block[ix_xyz];
            if (voxel > 0) voxel |= (light[ix_xyz] << 8);
            if (voxel != 0) {
              if (d==0) {
                if (not(w==kSize-1 || block[IX(int(xyz_next.x), int(xyz_next.y), int(xyz_next.z))]==0)) continue;
              } else {
                if (not(w==0 || block[IX(int(xyz_next.x), int(xyz_next.y), int(xyz_next.z))]==0)) continue;
              }
            }
            scratch[sidx] = voxel;
          }
        }

        // 2. Mesh using Scratch
        for (int u=0; u<kSize; u++) {
          for (int v=0; v<kSize; v++) {
            float uvw[] = { float(u), float(v), float(w) };
            glm::vec3 xyz = glm::vec3(uvw[x_idxs[aidx]], uvw[y_idxs[aidx]], uvw[z_idxs[aidx]]);
            glm::vec3 origin(-coord_min + xyz.x*kL0, -coord_min + xyz.y*kL0, -coord_min + xyz.z*kL0);
            const float l = 0.5f * kL0;
            const int voxel = scratch[u*kSize+v];
            if (voxel != 0) {                          //     V
              int du = 1, dv = 1;                     // P1 ----------- P0
              //       determine value of du and dv   // |  +W ������Ļ   |
              glm::vec3 pos_w = origin + l*w0,        // P2 ----------- P3 ---> U
                        p2 = pos_w - l*u0 - l*v0, p3 = p2 + float(du)*kL0*u0, p0 = p3 + float(dv)*kL0*v0,
                        p1 = p2 + float(dv)*kL0*v0;

              // +Z -Z +X -X +Y -Y
              const int ao_dirs[] = { 4,5,0,1,2,3 };
              const int ao_dir = ao_dirs[2*aidx+d];

              int ao_2 = GetOcclusionFactor(p2.x, p2.y, p2.z, ao_dir, neighbors);
              int ao_0 = 0, ao_1 = 0, ao_3 = 0;

              // ���� dv
              // Try possible dv values & make dv as large as possible
              bool may_extend_u = true;
              for (int ddv=1; ddv+v-1<kSize; ddv++) {
                int next_voxel = scratch[u*kSize+(v-1)+ddv];
                if (next_voxel != voxel) break;
                glm::vec3 p33 = p2 + float(du)*kL0*u0, p00 = p33 + float(ddv)*kL0*v0,
                          p11 = p2 + float(ddv)*kL0*v0;
                int ao_33 = GetOcclusionFactor(p33.x, p33.y, p33.z, ao_dir, neighbors),
                    ao_00 = GetOcclusionFactor(p00.x, p00.y, p00.z, ao_dir, neighbors),
                    ao_11 = GetOcclusionFactor(p11.x, p11.y, p11.z, ao_dir, neighbors);

                if (not (ao_33 == ao_2 && ao_11 == ao_2 && ao_00 == ao_2)) {
                  may_extend_u = false;
                  if (not (du==1 && ddv==1)) break;
                }

                ao_0 = ao_00; ao_1 = ao_11; ao_3 = ao_33;
                p0   = p00;   p1   = p11;   p3   = p33;
                dv = ddv;
                if (!may_extend_u) break;
              }

              if (may_extend_u) {
                for (int ddu = 2; ddu+u-1<kSize; ddu++) {
                  bool line_ok = true, checked = false;
                  glm::vec3 p11, p33, p00;
                  for (int vv=v; vv<v+dv; vv++) {
                    checked = true;
                    int the_voxel = scratch[(u-1+ddu)*kSize + vv];
                    if (the_voxel != voxel) {
                      line_ok = false; break;
                    }
                    p33 = p2 + float(ddu)*kL0*u0, p00 = p33 + float(vv-v+1)*kL0*v0,
                    p11 = p2 + float(vv-v+1)*kL0*v0;
                    int ao_33 = GetOcclusionFactor(p33.x, p33.y, p33.z, ao_dir, neighbors),
                        ao_00 = GetOcclusionFactor(p00.x, p00.y, p00.z, ao_dir, neighbors),
                        ao_11 = GetOcclusionFactor(p11.x, p11.y, p11.z, ao_dir, neighbors);
                    if (not (ao_33 == ao_2 && ao_11 == ao_2 && ao_00 == ao_2)) {
                      line_ok = false; break;
                    }
                  }
                  if (line_ok && checked) {
                    ao_0 = ao_1 = ao_3 = ao_2;
                    p0   = p00;   p1   = p11;   p3   = p33;
                    du   = ddu;
                  } else break;
                }
              }

              if (Chunk::verbose) {
                printf("p0=(%g,%g,%g) p1=(%g,%g,%g) p2=(%g,%g,%g) p3=(%g,%g,%g), ao=(%d,%d,%d,%d)\n",
                  p0.x, p0.y, p0.z,
                  p1.x, p1.y, p1.z,
                  p2.x, p2.y, p2.z,
                  p3.x, p3.y, p3.z,
                  ao_0, ao_1, ao_2, ao_3
                );
              }

              const float verts_xyz[] = {
                p3.x, p3.y, p3.z, p0.x, p0.y, p0.z, p2.x, p2.y, p2.z,
                p1.x, p1.y, p1.z, p2.x, p2.y, p2.z, p0.x, p0.y, p0.z
              };
              
              int idxes0[] = {
                0,1,2, 3,4,5, 6,7,8, 9,10,11, 12,13,14, 15,16,17, // (p3, p0, p2), (p1, p2, p0)
                0,1,2, 6,7,8, 3,4,5, 9,10,11, 15,16,17, 12,13,14, // (p3, p2, p0), (p1, p0, p2)
              };
              int idxes1[] = {
                3,4,5, 9,10,11, 0,1,2,   0,1,2, 9,10,11, 6,7,8,  // (p0, p1, p3), (p3, p1, p2)
                3,4,5, 0,1,2,   9,10,11, 0,1,2, 6,7,8,   9,10,11 // (p0, p3, p1), (p3, p2, p1)
              };

              int ao_factors0[] = {
                ao_3, ao_0, ao_2, ao_1, ao_2, ao_0,
                ao_3, ao_2, ao_0, ao_1, ao_0, ao_2
              };
              int ao_factors1[] = {
                ao_0, ao_1, ao_3, ao_3, ao_1, ao_2,
                ao_0, ao_3, ao_1, ao_3, ao_2, ao_1
              };
              int* idxes = idxes0;
              int* ao_factors = ao_factors0;

              if (ao_0 != ao_2 && ao_1 == ao_3) {
                idxes = idxes1;
                ao_factors = ao_factors1;
              }

              for (unsigned i=0; i<18; i++) {
                tmp_vert[idx_v++] = verts_xyz[idxes[i + d*18]];
              }
              for (unsigned i=0; i<6; i++) {
                tmp_norm[idx_n++] = aidx*2 + d;
              }

              for (int i=0; i<6; i++) {
                tmp_ao[idx_ao++]     = ao_factors[i + d*6];
                tmp_data[idx_data++] = voxel;
              }
              tri_count += 2;

              // Clear scratch
              for (int uu=u; uu<u+du; uu++)
                for (int vv=v; vv<v+dv; vv++)
                  scratch[uu*kSize+vv] = 0;
            }
          }
        }
      }
    }
  }

  glGenBuffers(1, &vbo);

  float* tmp_packed = new float[tri_count * 3 * 6];
  for (int i=0; i<tri_count*3; i++) {
    const float x=tmp_vert[i*3], y=tmp_vert[i*3+1], z=tmp_vert[i*3+2],
                nidx = tmp_norm[i],
                data = tmp_data[i],
                ao   = tmp_ao[i];
    const float entry[] = { x,y,z,nidx,data,ao };
    for (unsigned j=0; j<6; j++) {
      tmp_packed[i*6 + j] = entry[j];
    }
  }

  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glBufferData(GL_ARRAY_BUFFER, sizeof(float)*tri_count * 3 * 6,
    tmp_packed, GL_STATIC_DRAW);
  const size_t stride = sizeof(float)*6;

  glBindBuffer(GL_ARRAY_BUFFER, 0);

  delete[] tmp_vert; delete[] tmp_norm; delete[] tmp_data; delete[] tmp_ao;

  MyCheckError("Chunk::BuildBuffer");

  delete[] tmp_packed;
  is_dirty = false;
}

int Chunk::GetOcclusionFactor(const float x0, const float y0, const float z0, const int dir,
  Chunk* neighs[26]) {
  const float coord_min = kL0 * 0.5f;//(size) * l0 * 0.5f;
  const int xx = (x0 + coord_min) / kL0, yy = (y0 + coord_min) / kL0, zz = (z0 + coord_min) / kL0;
  int x_next[4], y_next[4], z_next[4];
  switch (dir) {
    case 0: // +X
      x_next[0] = x_next[1] = x_next[2] = x_next[3] = xx;
      y_next[0] = y_next[1] = yy; y_next[2] = y_next[3] = yy-1;
      z_next[0] = z_next[2] = zz; z_next[1] = z_next[3] = zz-1;
      break;
    case 1: // -X
      x_next[0] = x_next[1] = x_next[2] = x_next[3] = xx-1;
      y_next[0] = y_next[1] = yy; y_next[2] = y_next[3] = yy-1;
      z_next[0] = z_next[2] = zz; z_next[1] = z_next[3] = zz-1;
      break;
    case 2: // +Y
      y_next[0] = y_next[1] = y_next[2] = y_next[3] = yy;
      x_next[0] = x_next[1] = xx; x_next[2] = x_next[3] = xx-1;
      z_next[0] = z_next[2] = zz; z_next[1] = z_next[3] = zz-1;
      break;
    case 3: // -Y
      y_next[0] = y_next[1] = y_next[2] = y_next[3] = yy-1;
      x_next[0] = x_next[1] = xx; x_next[2] = x_next[3] = xx-1;
      z_next[0] = z_next[2] = zz; z_next[1] = z_next[3] = zz-1;
      break;
    case 4:
      z_next[0] = z_next[1] = z_next[2] = z_next[3] = zz;
      x_next[0] = x_next[1] = xx; x_next[2] = x_next[3] = xx-1;
      y_next[0] = y_next[2] = yy; y_next[1] = y_next[3] = yy-1;
      break;
    case 5:
      z_next[0] = z_next[1] = z_next[2] = z_next[3] = zz-1;
      x_next[0] = x_next[1] = xx; x_next[2] = x_next[3] = xx-1;
      y_next[0] = y_next[2] = yy; y_next[1] = y_next[3] = yy-1;
      break;
  }

  int occ = 0;
  for (int i=0; i<4; i++) {
    const int x1 = x_next[i], y1 = y_next[i], z1 = z_next[i];
    if (x1 < 0 || y1 < 0 || z1 < 0 ||
      x1 >= kSize || y1 >= kSize || z1 >= kSize) {
    if (x1 >= kSize) { // Cases [0] and [6:13]
      if (y1 < 0) {
        if (z1 < 0) {
          if (neighs[13] && neighs[13]->block[IX(0, kSize-1, kSize-1)]) occ++;
        } else if (z1 < kSize) {
          if (neighs[12] && neighs[12]->block[IX(0, kSize-1, z1)]) occ++;
        } else {
          if (neighs[11] && neighs[11]->block[IX(0, kSize-1, 0)]) occ++;
        }
      } else if (y1 < kSize) {
        if (z1 < 0) {
          if (neighs[10] && neighs[10]->block[IX(0, y1, kSize-1)]) occ++;
        } else if (z1 < kSize) {
          if (neighs[0] && neighs[0]->block[IX(0, y1, z1)]) occ++;
        } else {
          if (neighs[9] && neighs[9]->block[IX(0, y1, 0)]) occ++;
        }
      }
    } else if (x1 >= 0) { // Cases [2:5], [14:17]
      if (y1 < 0) {
        if (z1 < 0) {
          if (neighs[17] && neighs[17]->block[IX(x1, kSize-1, kSize-1)]) occ++;
        } else if (z1 < kSize) {
          if (neighs[3] && neighs[3]->block[IX(x1, kSize-1, z1)]) occ++;
        } else {
          if (neighs[16] && neighs[16]->block[IX(x1, kSize-1, 0)]) occ++;
        }
      } else if (y1 < kSize) {
        if (z1 < 0) {
          if (neighs[5] && neighs[5]->block[IX(x1, y1, kSize-1)]) occ++;
        } else if (z1 < kSize) {
          printf("ERROR: x1=%d, y1=%d, z1=%d\n", x1, y1, z1);
        } else {
          if (neighs[4] && neighs[4]->block[IX(x1, y1, 0)]) occ++;
        }
      } else {
        if (z1 < 0) {
          if (neighs[15] && neighs[15]->block[IX(x1, 0, kSize-1)]) occ++;
        } else if (z1 < kSize) {
          if (neighs[2] && neighs[2]->block[IX(x1, 0, z1)]) occ++;
        } else {
          if (neighs[14] && neighs[14]->block[IX(x1, 0, 0)]) occ++;
        }
      }
    } else { // Cases [1], [18:25]
      if (y1 < 0) {
        if (z1 < 0) {
          if (neighs[25] && neighs[25]->block[IX(kSize-1, kSize-1, kSize-1)]) occ++;
        } else if (z1 < kSize) {
          if (neighs[24] && neighs[24]->block[IX(kSize-1, kSize-1, z1)]) occ++;
        } else {
          if (neighs[23] && neighs[23]->block[IX(kSize-1, kSize-1, 0)]) occ++;
        }
      } else if (y1 < kSize) {
        if (z1 < 0) {
          if (neighs[22] && neighs[22]->block[IX(kSize-1, y1, kSize-1)]) occ++;
        } else if (z1 < kSize) {
          if (neighs[1] && neighs[1]->block[IX(kSize-1, y1, z1)]) occ++;
        } else {
          if (neighs[21] && neighs[21]->block[IX(kSize-1, y1, 0)]) occ++;
        }
      } else {
        if (z1 < 0) {
          if (neighs[20] && neighs[20]->block[IX(kSize-1, 0, kSize-1)]) occ++;
        } else if (z1 < kSize) {
          if (neighs[19] && neighs[19]->block[IX(kSize-1, 0, z1)]) occ++;
        } else {
          if (neighs[18] && neighs[18]->block[IX(kSize-1, 0, 0)]) occ++;
        }
      }
    }
  } else {
      if (block[IX(x1, y1, z1)] != 0) occ++;
    }
  }
  return occ;
}

void Chunk::LoadDefault() {
  for (int x=0; x<kSize; x++) {
    for (int y=0; y<kSize; y++) {
      for (int z=0; z<kSize; z++) {
        int dx = kSize/2-x, dy=kSize/2-y, dz=kSize/2-z;
        if (dx*dx + dy*dy + dz*dz <= 10*10)
          block[IX(x,y,z)] = 120;//(x*101+y*47+z*119) % 255;
      }
    }
  }
}

void Chunk::Render(const glm::mat4& M) {
  glUseProgram(shader_program);
  GLuint mLoc = glGetUniformLocation(shader_program, "M");
  glUniformMatrix4fv(mLoc, 1, GL_FALSE, &(M[0][0]));

  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  const size_t stride = sizeof(float)*6;
  // XYZ pos
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (GLvoid*)0);
  glEnableVertexAttribArray(0);

  // Normal idx + Data + AO Index
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (GLvoid*)(3*sizeof(GLfloat)));
  glEnableVertexAttribArray(1);

  glDrawArrays(GL_TRIANGLES, 0, tri_count * 3);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glUseProgram(0);
  MyCheckError("Chunk::Render");
}

void Chunk::Render() {
  glm::mat4 M(1);
  M = glm::translate(M, pos);
  Render(M);
}

void Chunk::SetVoxel(unsigned x, unsigned y, unsigned z, int v) {
  block[IX(x,y,z)] = v;
  is_dirty = true;
}

int Chunk::GetVoxel(unsigned x, unsigned y, unsigned z) {
  return block[IX(x,y,z)];
}