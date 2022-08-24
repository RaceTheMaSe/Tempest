#pragma once

#include <cstring>

namespace Tempest{

  //! Color, rgba, [0..1], float precision.
  class Color {
    public:
      //! Constructor. Equals Color(0.0).
      Color()=default;

      //! Also a constructor. All channels set by rgba.
      Color( float rgba ){ cdata[0]=rgba;cdata[1]=rgba;cdata[2]=rgba;cdata[3]=rgba; }

      //! Constructor.
      Color( float r, float g,
             float b, float a = 1.0 ){cdata[0]=r;cdata[1]=g;cdata[2]=b;cdata[3]=a;}
      Color(const Color&)=default;

      //! Assignment.
      Color& operator = ( const Color & other)=default;


      //! Set all channels explicitly.
      void set(float r,
               float g,
               float b,
               float a){
        cdata[0]=r;cdata[1]=g;cdata[2]=b;cdata[3]=a;
        }
      //! Overloaded function, introduced for convenience
      void set( float rgba ){
        cdata[0]=rgba;cdata[1]=rgba;cdata[2]=rgba;cdata[3]=rgba;
        }

      //! Returns a float[4] array with the rgba channel values
      const float * data() const { return (const float*)cdata; }

      //! Add. The result will not be clamped
      friend Color operator + ( Color l,const Color & r ){ l+=r; return l; }
      Color&  operator += ( const Color & other) {
        cdata[0]+=other.cdata[0];
        cdata[1]+=other.cdata[1];
        cdata[2]+=other.cdata[2];
        cdata[3]+=other.cdata[3];
        return *this;
        }
      //! Substraction. The result will not be clamped
      friend Color operator - ( Color l,const Color & r ){ l-=r; return l; }
      Color&  operator -= ( const Color & other) {
        cdata[0]-=other.cdata[0];
        cdata[1]-=other.cdata[1];
        cdata[2]-=other.cdata[2];
        cdata[3]-=other.cdata[3];
        return *this;
        }

      //! Channel red.
      float r() const { return cdata[0]; }
      //! Channel green.
      float g() const { return cdata[1]; }
      //! Channel blue.
      float b() const { return cdata[2]; }
      //! alpha channel.
      float a() const { return cdata[3]; }

      float& operator[]( int i )  { return cdata[i]; }
      const float& operator[]( int i ) const  { return cdata[i]; }

      bool operator == ( const Color & other ) const {
        return std::memcmp(this,&other,sizeof(*this))==0;
        }

      bool operator !=( const Color & other ) const {
        return std::memcmp(this,&other,sizeof(*this))!=0;
        }
    private:
      float cdata[4]={};
  };

}
