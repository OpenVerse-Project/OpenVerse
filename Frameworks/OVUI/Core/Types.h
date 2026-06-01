#pragma once

#include <cstdint>
#include <cmath>
#include <algorithm>
#include <string>
#include <vector>
#include <cstring>

namespace ovui {

using WidgetID = uint64_t;
constexpr WidgetID INVALID_WIDGET = 0;

struct Point { float x=0, y=0; };
struct Size { float width=0, height=0; };
struct Rect { float x=0, y=0, width=0, height=0;
    float right()const{return x+width;} float bottom()const{return y+height;}
    Point center()const{return{x+width*0.5f,y+height*0.5f};}
    bool contains(Point p)const{return p.x>=x&&p.x<=right()&&p.y>=y&&p.y<=bottom();}
    Rect intersect(const Rect& o)const{
        float l=std::max(x,o.x),t=std::max(y,o.y),r=std::min(right(),o.right()),b=std::min(bottom(),o.bottom());
        if(l>=r||t>=b)return{};
        return{l,t,r-l,b-t};
    }
};

struct EdgeInsets { float top=0,right=0,bottom=0,left=0;
    float horizontal()const{return left+right;} float vertical()const{return top+bottom;}
    Rect inset(const Rect& r)const{return{r.x+left,r.y+top,r.width-left-right,r.height-top-bottom};}
};

struct Color {
    float r=0,g=0,b=0,a=1;
    static Color from_rgba(uint8_t r,uint8_t g,uint8_t b,uint8_t a=255){return{r/255.f,g/255.f,b/255.f,a/255.f};}
    static Color from_hex(uint32_t hex){return from_rgba((hex>>24)&0xff,(hex>>16)&0xff,(hex>>8)&0xff,hex&0xff);}
    Color darken(float f)const{return{r*std::max(0.f,1.f-f),g*std::max(0.f,1.f-f),b*std::max(0.f,1.f-f),a};}
    Color lighten(float f)const{float s=1.f+f;return{std::min(1.f,r*s),std::min(1.f,g*s),std::min(1.f,b*s),a};}
    bool operator==(const Color&o)const{return r==o.r&&g==o.g&&b==o.b&&a==o.a;}
};

struct Matrix3x3 { float m[9]={1,0,0,0,1,0,0,0,1};
    static Matrix3x3 identity(){return{};}
    static Matrix3x3 translate(float x,float y){Matrix3x3 r;r.m[6]=x;r.m[7]=y;return r;}
    static Matrix3x3 scale(float sx,float sy){Matrix3x3 r;r.m[0]=sx;r.m[4]=sy;return r;}
    Point transform(Point p)const{return{m[0]*p.x+m[3]*p.y+m[6],m[1]*p.x+m[4]*p.y+m[7]};}
};

enum class Cursor { Arrow, IBeam, Hand, ResizeH, ResizeV, ResizeNWSE, ResizeNESW, Move, NotAllowed, Wait };

enum class WidgetState : uint32_t {
    None=0, Hover=1<<0, Pressed=1<<1, Focused=1<<2, Disabled=1<<3,
    Active=1<<4, Selected=1<<5, Dragging=1<<6, Error=1<<7,
};

inline WidgetState operator|(WidgetState a,WidgetState b){return WidgetState(uint32_t(a)|uint32_t(b));}
inline WidgetState operator&(WidgetState a,WidgetState b){return WidgetState(uint32_t(a)&uint32_t(b));}
inline WidgetState operator~(WidgetState a) { return WidgetState(~uint32_t(a)); }
inline bool operator!(WidgetState a){return uint32_t(a)==0;}

class ArenaAllocator {
    uint8_t* m_buffer=nullptr;size_t m_cap=0,m_off=0;
public:
    ArenaAllocator(size_t cap=1024*1024):m_cap(cap){m_buffer=new uint8_t[cap];}
    ~ArenaAllocator(){delete[]m_buffer;}
    void* alloc(size_t sz,size_t al=8){
        size_t ao=(m_off+al-1)&~(al-1);
        if(ao+sz>m_cap)return nullptr;
        void*p=m_buffer+ao;m_off=ao+sz;return p;
    }
    void reset(){m_off=0;}
    size_t used()const{return m_off;}
};

} // namespace ovui
