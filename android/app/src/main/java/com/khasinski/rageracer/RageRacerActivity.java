package com.khasinski.rageracer;

import org.libsdl.app.SDLActivity;

/**
 * SDL is linked into libmain.so, so the loader must not also open libSDL3.so.
 * A .cue, .bin, or .chd belongs in the app external files directory, or in
 * one folder directly under it.
 */
public class RageRacerActivity extends SDLActivity {
    @Override
    protected String[] getLibraries() {
        return new String[] { "main" };
    }
}
