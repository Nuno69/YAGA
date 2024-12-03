// Supported with union (c) 2018-2022 Union team

#ifndef __STEAMWORKS_H__
#define __STEAMWORKS_H__

namespace UnionCore {
  class oSteamworks {
    oSteamworks() {
#if ENGINE == Engine_G1 || ENGINE == Engine_G2A
      if( SteamOverlayIsEnabled() ) {
        if( LoadLibraryAST( "Steam_Api.dll" ) ) {
          Steamworks = LoadLibraryAST( "oSteamworks.dll" );
          return;
        }
      }
#endif
      Steamworks = Null;
    }
  public:
    HMODULE Steamworks;
    void OpenUrl( const char* url ) {
      if( Steamworks ) {
        static auto proc = (void(*)(const char* url))GetProcAddress( Steamworks, "os_OpenUrl" );
        cmd << AHEX32( proc ) << endl;
        if( proc )
          return proc( url );
      }

      system( "start " + A url );
    }

    const char* GetPersonalName() {
      if( Steamworks ) {
        static auto proc = (const char* (*)())GetProcAddress( Steamworks, "os_GetPersonalName" );
        if( proc )
          return proc();
      }

      return "";
    }

    static bool SteamOverlayIsEnabled() {
      return
        GetModuleHandle( "GameOverlayRenderer.dll" ) ||
        GetModuleHandle( "GameOverlayRenderer.dll.tmp" );
    }

    static oSteamworks& GetInstance() {
      static oSteamworks instance;
      return instance;
    }
  };
}

#endif // __STEAMWORKS_H__