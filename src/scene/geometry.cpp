#include "terrapainter/scene/geometry.h"

Geometry::Geometry() :
  indices(), primitive(GL_TRIANGLES) { }

// Triangle Strip primitive
Geometry::Geometry(int height, int width, int numTrisPerStrips, int strips) {
  stripData = StripData { height, width, numTrisPerStrips, strips };
  primitive = GL_TRIANGLE_STRIP;
}

void Geometry::setAttr(std::string name, Attribute* attr) {
  attrs[name] = attr;
}

void Geometry::setIndex(std::vector<unsigned int> idx) {
  indices = idx;
}

bool Geometry::hasIndices() {
  return (indices.has_value());
}

// Returns NULL if not found
Attribute* Geometry::getAttr(std::string name) {
  if (hasAttr(name)) {
    auto a = attrs[name];
    return a;
  } else { return NULL; }
}

bool Geometry::hasAttr(std::string name) {
  return attrs.contains(name);
}

// Expects an attribute named "normal"
void Geometry::normalizeNormals() {
  Attribute* normals = this->getAttr( "normal" );

  if (normals) {
    auto il = normals->count;
    for ( auto i = 0; i < il; i ++ ) {
      vec3 n = vec3::splat(0.0);
      normals->getXYZ(i, n);
      n = n.normalize();
      normals->setXYZ( i, n.x, n.y, n.z );
    }
  }
}

// Expects an attribute named "normal"
void Geometry::normalizeTangents() {
  Attribute* tangents = this->getAttr( "tangent" );

  if (tangents) {
    auto il = tangents->count;
    for ( auto i = 0; i < il; i ++ ) {
      vec3 n = vec3::splat(0.0);
      tangents->getXYZ(i, n);
      n = n.normalize();
      tangents->setXYZ( i, n.x, n.y, n.z );
    }
  }
}

void Geometry::GenerateNormalTangent() {
  Attribute* posAttr = Geometry::getAttr( "position" );

  if ( posAttr ) {

    if ( !Geometry::hasAttr("normal") ) {
      // create normals attribute
      std::vector<float> norms(posAttr->count * 3);
      Attribute* nAttr = new Attribute( &norms, 3 );
      Geometry::setAttr( "normal", nAttr );
    } else {
      // reset existing normals to zero
      unsigned int il = Geometry::getAttr( "normal" )->count;
      for ( auto i = 0; i < il; i ++ ) {
        Geometry::getAttr( "normal" )->setXYZ( i, 0.f, 0.f, 0.f );
      }
    }

    if ( !Geometry::hasAttr("tangent") ) {
      // create tangent attribute
      std::vector<float> tangs(posAttr->count * 3);
      Attribute* tAttr = new Attribute( &tangs, 3 );
      Geometry::setAttr( "tangent", tAttr );
    } else {
      // reset existing tangent to zero
      unsigned int il = Geometry::getAttr( "tangent" )->count;
      for ( auto i = 0; i < il; i ++ ) {
        Geometry::getAttr( "tangent" )->setXYZ( i, 0.f, 0.f, 0.f );
      }
    }

    Attribute* normalAttr = Geometry::getAttr( "normal" );

    // indexed elements (smooth shading)
    if ( Geometry::hasIndices() ) {
      switch (primitive)
      {
      case GL_TRIANGLES:
        Geometry::GenerateNormalTangentTriangle();
        break;

      case GL_TRIANGLE_STRIP:
        Geometry::GenerateNormalTangentStrips();
        break;

      default:
        assert(0);
        break;
      }
    } else {
      // non-indexed elements (flat shading)
      auto il = posAttr->count;
      for ( auto i = 0; i < il; i += 3 ) {

        vec3 pA = vec3::splat(0.0);
        posAttr->getXYZ(i, pA);

        vec3 pB = vec3::splat(0.0);
        posAttr->getXYZ(i+1, pB);

        vec3 pC = vec3::splat(0.0);
        posAttr->getXYZ(i+2, pC);

        vec3 cb = pC - pB;
        vec3 ab = pA - pB;
        vec3 n = cross(cb, ab);

        normalAttr->setXYZ( i + 0, n.x, n.y, n.z );
        normalAttr->setXYZ( i + 1, n.x, n.y, n.z );
        normalAttr->setXYZ( i + 2, n.x, n.y, n.z );
      }
    }
    Geometry::normalizeNormals();
    Geometry::normalizeTangents();

    normalAttr->needsUpdate = true;
  }
}

void Geometry::GenerateNormalTangentTriangle() {
  Attribute* posAttr = Geometry::getAttr( "position" );
  Attribute* normalAttr = Geometry::getAttr( "normal" );

  std::vector<unsigned int>& index = indices.value();
  unsigned int il = index.size();

  for ( auto i = 0; i < il; i += 3 ) {
    auto vA = index.at(i);
    auto vB = index.at(i + 1);
    auto vC = index.at(i + 2);

    // Get positions of triangle
    vec3 pA = vec3::splat(0.0);
    posAttr->getXYZ(vA, pA);

    vec3 pB = vec3::splat(0.0);
    posAttr->getXYZ(vB, pB);

    vec3 pC = vec3::splat(0.0);
    posAttr->getXYZ(vC, pC);

    vec3 cb = pC - pB;
    vec3 ab = pA - pB;
    vec3 norm = cross(cb, ab);

    vec3 nA = vec3::splat(0.0);

    // printf("count: %d, t_size: %d, size: %d", normalAttr->count, normalAttr->t_size, normalAttr->size);

    normalAttr->getXYZ(vA, nA);

    vec3 nB = vec3::splat(0.0);
    normalAttr->getXYZ(vB, nB);

    vec3 nC = vec3::splat(0.0);
    normalAttr->getXYZ(vC, nC);

    nA += cb;
    nB += cb;
    nC += cb;

    normalAttr->setXYZ( vA, nA.x, nA.y, nA.z );
    normalAttr->setXYZ( vB, nB.x, nB.y, nB.z );
    normalAttr->setXYZ( vC, nC.x, nC.y, nC.z );
  }
}

void Geometry::GenerateNormalTangentStrips() {
  Attribute* posAttr = Geometry::getAttr( "position" );
  Attribute* normalAttr = Geometry::getAttr( "normal" );
  Attribute* tangAttr = Geometry::getAttr( "tangent" );

  StripData dat = stripData.value(); 

  // facedata[i] is the vertex index for face i // 3
  std::vector<unsigned int> facedata;
  // loading each face in
  for (int i = 0; i < dat.height - 1; i++)
  {
      for (int j = 0; j < dat.width - 1; j++)
      {
          facedata.push_back(i * dat.width + j);
          facedata.push_back(i * dat.width + j + 1);
          facedata.push_back((i + 1) * dat.width + j);
          facedata.push_back(i * dat.width + j + 1);
          facedata.push_back((i + 1) * dat.width + j + 1);
          facedata.push_back((i + 1) * dat.width + j);
      }
  }

  for (int i = 0; i < facedata.size(); i += 3)
  {
      auto vA = facedata.at(i);
      auto vB = facedata.at(i + 1);
      auto vC = facedata.at(i + 2);

      // Get positions of triangle
      vec3 v1 = vec3::splat(0.0);
      posAttr->getXYZ(vA, v1);

      vec3 v2 = vec3::splat(0.0);
      posAttr->getXYZ(vB, v2);

      vec3 v3 = vec3::splat(0.0);
      posAttr->getXYZ(vC, v3);

      vec3 side1 = v2 - v1;
      vec3 side2 = v3 - v1;
      vec3 normal = cross(side1, side2);

      vec3 tangent;
      if (i % 2 == 0) {tangent = side1;}
      else { tangent = v2 - v3; }

      vec3 nA = vec3::splat(0.0);
      normalAttr->getXYZ(vA, nA);

      vec3 nB = vec3::splat(0.0);
      normalAttr->getXYZ(vB, nB);

      vec3 nC = vec3::splat(0.0);
      normalAttr->getXYZ(vC, nC);

      nA += normal;
      nB += normal;
      nC += normal;
      
      normalAttr->setXYZ( vA, nA.x, nA.y, nA.z );
      normalAttr->setXYZ( vB, nB.x, nB.y, nB.z );
      normalAttr->setXYZ( vC, nC.x, nC.y, nC.z );

      vec3 tA = vec3::splat(0.0);
      tangAttr->getXYZ(vA, tA);

      vec3 tB = vec3::splat(0.0);
      tangAttr->getXYZ(vB, tB);

      vec3 tC = vec3::splat(0.0);
      tangAttr->getXYZ(vC, tC);

      tA += tangent;
      tB += tangent;
      tC += tangent;
      
      tangAttr->setXYZ( vA, tA.x, tA.y, tA.z );
      tangAttr->setXYZ( vB, tB.x, tB.y, tB.z );
      tangAttr->setXYZ( vC, tC.x, tC.y, tC.z );
  }
}