#include "living_village/sprites.hpp"
#include <algorithm>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>
namespace village {
Sprites::Sprites(SDLApi& api, SDL_Renderer* target) : sdl(api), renderer(target) {
    // Original hand-authored pixel silhouettes; all textures are built once.
    const std::vector<std::vector<std::string>> art{
        {"........gg......","......ggGGgg....","....ggGLGGLgg...","...gGLLGGGLGg...","..gGLLGGGGGGGg..",".gGLGGGGGGGGDGg.","gGLGGGGGGGGDDDg.","gGGGGGGGGDDDDDg.",".gGGGGGGDDDDDDg.","..ggGGGDDDgggg..","....ggwwWgg.....","......wWW.......","......wWW.......",".....wwWWw......","....ddwWWdd.....","...dddddddd....."},
        {".......L........","......gGg.......",".....gGLGg......","....gGGLGGg.....","...gGGGGDDGg....",".....gGLGg......","....gGGLGGg.....","...gGGLGGGGg....","..gGGGGGGDDDg...",".gGGGGGGDDDDDGg.","....gGLGGGg.....","...gGGLGGGGg....","..gGGGGGGDDDGg..",".gGGGGGGDDDDDDg.",".......WW.......","......wWWw......"},
        {".........aa.a...",".........aSa....","........aaa.....","....aaaaaaEa....","...aSSSaaaaa....","..aSSSSaaaaN....","...aaaaaaa......","....a..a........","...aa..aa......."},
        {"............aa..","...........aSa..","..........aaaE..","..........aaaN..","...aaaaaaaaw....","..aSSSSaaaaaw...",".aSSSSaaaaaa....","..aaaaaaaaaa....","...w..w..w.w....","...w..w..w.w....","...d..d..d.d...."},
        {"..........o..o..","..........oSo...","...oooooooEo....","..oSSoooooooN...",".oSSSoooooo.....","SoSooooooo......","SS....d..d......","......d..d......"},
        {"...SS...SS......","..SSSS.SSSS.....","...SSSySSS......","....yEyEy.......",".....yyy........","......d........."},
        {".....hhhh.......","....hHHHHh......","....hSSSSh......",".....SESSh......","......SS........","....cccccc......","...ccccLccc.....","...SccLLccS.....","...SccccccS.....","....cccccc......",".....pppp.......",".....p..p.......","....dd..dd......"},
        {"........rr......",".......rRRr.....","......rRLRRr....",".....rRLRRRRr...","....rRLRRRRRRr..","...rRLRRRRRRRRr.","..rRRRRRRRRRRRRr","..rrrrrrrrrrrrrr","...WWWWWWWWWWW..","...WLWWWWWWWWW..","...WSSWWWWSSWW..","...WSEWWWWESWW..","...WWWWddWWWWW..","...WWWWddWWWWW..","...WWWWddWWWWW..","..dddddddddddd.."}
    };
    auto color = [](char key, int kind) -> Uint32 {
        SDL_Color c{};
        switch (key) {
            case 'g': c={27,55,45,255}; break; case 'G': c={53,100,63,255}; break;
            case 'L': c={136,165,87,255}; break; case 'D': c={36,76,54,255}; break;
            case 'w': c={80,58,43,255}; break; case 'W': c={175,132,81,255}; break;
            case 'd': c={35,42,38,255}; break; case 'a': c=kind==2 ? SDL_Color{196,182,152,255}:SDL_Color{158,109,64,255}; break;
            case 'o': c={195,105,53,255}; break; case 'S': c=kind==6 ? SDL_Color{236,185,137,255}:SDL_Color{245,228,187,255}; break;
            case 'E': c={25,31,31,255}; break; case 'N': c={52,48,39,255}; break;
            case 'y': c={228,177,59,255}; break; case 'h': c={63,48,37,255}; break;
            case 'H': c={104,74,45,255}; break; case 'c': c={104,165,185,255}; break;
            case 'p': c={65,65,76,255}; break; case 'r': c={90,50,43,255}; break;
            case 'R': c={162,81,60,255}; break; default: c={0,0,0,0}; break;
        }
        return (static_cast<Uint32>(c.r)<<24) | (static_cast<Uint32>(c.g)<<16) | (static_cast<Uint32>(c.b)<<8) | c.a;
    };
    try {
        for (std::size_t i=0; i<art.size(); ++i) {
            const int h=static_cast<int>(art[i].size()), w=16; std::vector<Uint32> pixels(w*h,0);
            for (int y=0;y<h;++y) for (int x=0;x<std::min(w,static_cast<int>(art[i][y].size()));++x) pixels[y*w+x]=color(art[i][y][x],static_cast<int>(i));
            textures[i]=sdl.CreateTexture(renderer,SDL_PIXELFORMAT_RGBA8888,SDL_TEXTUREACCESS_STATIC,w,h);
            if (!textures[i] || sdl.UpdateTexture(textures[i],nullptr,pixels.data(),w*sizeof(Uint32)) != 0) throw std::runtime_error(sdl.GetError());
            sdl.SetTextureBlendMode(textures[i],SDL_BLENDMODE_BLEND);
        }
    } catch (...) { for(auto* t:textures) if(t) sdl.DestroyTexture(t); throw; }
}
Sprites::~Sprites() { for(auto* t:textures) if(t) sdl.DestroyTexture(t); }
void Sprites::draw(Sprite sprite,int x,int y,int width,int height,bool mirror,SDL_Color tint) {
    auto* t=textures[static_cast<int>(sprite)]; sdl.SetTextureColorMod(t,tint.r,tint.g,tint.b);
    SDL_Rect to{x,y,width,height}; sdl.RenderCopyEx(renderer,t,nullptr,&to,0,nullptr,mirror ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE);
}
}
