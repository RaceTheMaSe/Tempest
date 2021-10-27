#pragma once

#include <cstdint>
#include <cmath>
#include <algorithm>
#include <cassert>

namespace Tempest {

template<class T,size_t len>
class BasicPoint;

template<class T>
struct BasicSize;

template<class T>
struct BasicRect;

enum Orientation:uint8_t {
  Horizontal=0,
  Vertical  =1
  };

template<class T>
class BasicPoint<T,1> {
  public:
    BasicPoint()=default;
    BasicPoint(T xIn):x(xIn){}

    BasicPoint& operator -= ( const BasicPoint & p ){ x-=p.x; return *this; }
    BasicPoint& operator += ( const BasicPoint & p ){ x+=p.x; return *this; }

    BasicPoint& operator /= ( const T& p ){ x/=p; return *this; }
    BasicPoint& operator *= ( const T& p ){ x*=p; return *this; }

    friend BasicPoint operator - ( BasicPoint l,const BasicPoint & r ){ l-=r; return l; }
    friend BasicPoint operator + ( BasicPoint l,const BasicPoint & r ){ l+=r; return l; }

    friend BasicPoint operator / ( BasicPoint l,const T r ){ l/=r; return l; }
    friend BasicPoint operator * ( BasicPoint l,const T r ){ l*=r; return l; }

    BasicPoint operator - () const { return BasicPoint(-x); }

    T length() const { return x<T() ? -x : x; }
    T quadLength()      const { return x*x; }

    T operator[](size_t idx) {
      assert(idx<1);
      switch(idx) {
        case 0: return x;
        }
      return T(-1);
      }

    bool operator ==( const BasicPoint & other ) const { return x==other.x; }
    bool operator !=( const BasicPoint & other ) const { return x!=other.x; }

    static T dotProduct(const BasicPoint<T,1>& a,const BasicPoint<T,1>& b) { return a.x*b.x; }
    static BasicPoint<T,3> normalize(const BasicPoint<T,3>& t) {
      if(t.x==T())
        return t;
      return T(1);
      }

    T x=T();
  };

template<class T>
class BasicPoint<T,2> {
  public:
    BasicPoint()=default;
    BasicPoint(T xIn,T yIn):x(xIn),y(yIn){}

    BasicPoint& operator -= ( const BasicPoint & p ){ x-=p.x; y-=p.y; return *this; }
    BasicPoint& operator += ( const BasicPoint & p ){ x+=p.x; y+=p.y; return *this; }

    BasicPoint& operator /= ( const T& p ){ x/=p; y/=p; return *this; }
    BasicPoint& operator *= ( const T& p ){ x*=p; y*=p; return *this; }

    friend BasicPoint operator - ( BasicPoint l,const BasicPoint & r ){ l-=r; return l; }
    friend BasicPoint operator + ( BasicPoint l,const BasicPoint & r ){ l+=r; return l; }

    friend BasicPoint operator / ( BasicPoint l,const T r ){ l/=r; return l; }
    friend BasicPoint operator * ( BasicPoint l,const T r ){ l*=r; return l; }

    BasicPoint operator - () const { return BasicPoint(-x,-y); }

    T length() const { return T(std::sqrt(x*x+y*y)); }
    T quadLength()      const { return x*x+y*y; }

    bool operator ==( const BasicPoint & other ) const { return x==other.x && y==other.y; }
    bool operator !=( const BasicPoint & other ) const { return x!=other.x || y!=other.y; }

    T operator[](size_t idx) {
      assert(idx<2);
      switch(idx) {
        case 0: return x;
        case 1: return y;
        }
      return T(-1);
      }

    static T dotProduct(const BasicPoint<T,2>& a,const BasicPoint<T,2>& b) { return a.x*b.x+a.y*b.y; }
    static BasicPoint<T,2> crossProduct(const BasicPoint<T,2>& a) {
      return { a.y, -a.x };
      }
    static BasicPoint<T,2> normalize(const BasicPoint<T,2>& t) {
      const T len = t.length();
      if(len==T())
        return t;
      return t/len;
      }

    T x=T();
    T y=T();
  };

template<class T>
class BasicPoint<T,3> {
  public:
    BasicPoint()=default;
    BasicPoint(T xIn,T yIn,T zIn):x(xIn),y(yIn),z(zIn){}

    BasicPoint& operator -= ( const BasicPoint & p ){ x-=p.x; y-=p.y; z-=p.z; return *this; }
    BasicPoint& operator += ( const BasicPoint & p ){ x+=p.x; y+=p.y; z+=p.z; return *this; }

    BasicPoint& operator /= ( const T& p ){ x/=p; y/=p; z/=p; return *this; }
    BasicPoint& operator *= ( const T& p ){ x*=p; y*=p; z*=p; return *this; }

    friend BasicPoint operator - ( BasicPoint l,const BasicPoint & r ){ l-=r; return l; }
    friend BasicPoint operator + ( BasicPoint l,const BasicPoint & r ){ l+=r; return l; }

    friend BasicPoint operator / ( BasicPoint l,const T r ){ l/=r; return l; }
    friend BasicPoint operator * ( BasicPoint l,const T r ){ l*=r; return l; }

    BasicPoint operator - () const { return BasicPoint(-x,-y,-z); }

    T length() const { return T(std::sqrt(x*x+y*y+z*z)); }
    T quadLength()      const { return x*x+y*y+z*z; }

    bool operator ==( const BasicPoint & other ) const { return x==other.x && y==other.y && z==other.z; }
    bool operator !=( const BasicPoint & other ) const { return x!=other.x || y!=other.y || z!=other.z; }
    bool operator < ( const BasicPoint & other ) const { return x<other.x && y<other.y && z<other.z; }
    bool operator > ( const BasicPoint & other ) const { return x>other.x && y>other.y && z>other.z; }

    T operator[](size_t idx) {
      assert(idx<3);
      switch(idx) {
        case 0: return x;
        case 1: return y;
        case 2: return z;
        }
      return T(-1);
      }

    static T dotProduct(const BasicPoint<T,3>& a,const BasicPoint<T,3>& b) { return a.x*b.x+a.y*b.y+a.z*b.z; }
    static BasicPoint<T,3> crossProduct(const BasicPoint<T,3>& a,const BasicPoint<T,3>& b) {
      return {
        a.y*b.z - a.z*b.y,
        a.z*b.x - a.x*b.z,
        a.x*b.y - a.y*b.x
        };
      }
    static BasicPoint<T,3> normalize(const BasicPoint<T,3>& t) {
      const T len = t.length();
      if(len==T())
        return t;
      return t/len;
      }

    T x=T();
    T y=T();
    T z=T();
  };

template<class T>
class BasicPoint<T,4> {
  public:
    BasicPoint()=default;
    BasicPoint(T xIn,T yIn,T zIn,T wIn):x(xIn),y(yIn),z(zIn),w(wIn){}

    BasicPoint& operator -= ( const BasicPoint & p ){ x-=p.x; y-=p.y; z-=p.z; w-=p.w; return *this; }
    BasicPoint& operator += ( const BasicPoint & p ){ x+=p.x; y+=p.y; z+=p.z; w+=p.w; return *this; }

    BasicPoint& operator /= ( const T& p ){ x/=p; y/=p; z/=p; w/=p; return *this; }
    BasicPoint& operator *= ( const T& p ){ x*=p; y*=p; z*=p; w*=p; return *this; }

    friend BasicPoint operator - ( BasicPoint l,const BasicPoint & r ){ l-=r; return l; }
    friend BasicPoint operator + ( BasicPoint l,const BasicPoint & r ){ l+=r; return l; }

    friend BasicPoint operator / ( BasicPoint l,const T r ){ l/=r; return l; }
    friend BasicPoint operator * ( BasicPoint l,const T r ){ l*=r; return l; }

    BasicPoint operator - () const { return BasicPoint(-x,-y,-z,-w); }

    T length() const { return T(std::sqrt(x*x+y*y+z*z+w*w)); }
    T quadLength()      const { return x*x+y*y+z*z+w*w; }

    bool operator ==( const BasicPoint & other ) const { return x==other.x && y==other.y && z==other.z && w==other.w; }
    bool operator !=( const BasicPoint & other ) const { return x!=other.x || y!=other.y || z!=other.z || w!=other.w; }
    
    T operator[](size_t idx) {
      assert(idx<4);
      switch(idx) {
        case 0: return x;
        case 1: return y;
        case 2: return z;
        case 3: return w;
        }
      return T(-1);
      }

    static T dotProduct(const BasicPoint<T,4>& a,const BasicPoint<T,4>& b) { return a.x*b.x+a.y*b.y+a.z*b.z+a.w*b.w; }
    static BasicPoint<T,4> normalize(const BasicPoint<T,4>& t) {
      const T len = t.length();
      if(len==T())
        return t;
      return t/len;
      }

    T x=T();
    T y=T();
    T z=T();
    T w=T();
  };

template<class T>
struct BasicSize {
  explicit BasicSize(T is = T()):w(is), h(is) {}
  BasicSize(T ix, T iy):w(ix), h(iy) {}

  T w=T(), h=T();

  BasicPoint<T,2> toPoint() const { return BasicPoint<T,2>{w,h};      }
  BasicRect<T>    toRect()  const { return BasicRect<T>{T(),T(),w,h}; }
  bool            isEmpty() const { return w<=0 || h<=0;              }

  bool operator ==( const BasicSize & other ) const { return w==other.w && h==other.h; }
  bool operator !=( const BasicSize & other ) const { return w!=other.w || h!=other.h; }
  };

template<class T>
struct BasicRect {
  BasicRect()=default;
  BasicRect( T ix,
             T iy,
             T iw,
             T ih )
    :x(ix), y(iy), w(iw), h(ih) {}
  T x=T(), y=T(), w=T(), h=T();

  BasicPoint<T,2> pos()  const { return BasicPoint<T,2>{x,y}; }
  BasicSize<T>    size() const { return BasicSize<T>(w,h);    }

  BasicRect intersected(const BasicRect& r) const {
    BasicRect re;
    re.x = std::max( x, r.x );
    re.y = std::max( y, r.y );

    re.w = std::min( x+w, r.x+r.w ) - re.x;
    re.h = std::min( y+h, r.y+r.h ) - re.y;

    re.w = std::max(0, re.w);
    re.h = std::max(0, re.h);

    return re;
    }

  bool contains( const BasicPoint<T,2> & p ) const {
    return contains(p.x, p.y);
    }
  bool contains( const T& px,const T& py ) const {
    return ( x<px && px<x+w ) && ( y<py && py<y+h );
    }

  bool contains( const BasicPoint<T,2> & p, bool border ) const {
    return contains(p.x, p.y,border);
    }
  bool contains( const T& px,const T& py, bool border ) const {
    if( border )
      return ( x<=px && px<=x+w ) && ( y<=py && py<=y+h ); else
      return contains(x,y);
    }

  bool isEmpty() const { return w<=0 || h<=0; }

  bool operator ==( const BasicRect& other ) const { return x==other.x && y==other.y && w==other.w && h==other.h; }
  bool operator !=( const BasicRect& other ) const { return x!=other.x || y!=other.y || w!=other.w || h!=other.h; }
  };

using Point  = BasicPoint<int,2>;
using PointF = BasicPoint<float,2>;

using Vec1   = BasicPoint<float,1>;
using Vec2   = BasicPoint<float,2>;
using Vec3   = BasicPoint<float,3>;
using Vec4   = BasicPoint<float,4>;

using Size   = BasicSize<int>;
using Rect   = BasicRect<int>;
}
