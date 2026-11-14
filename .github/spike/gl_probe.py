import ctypes
import sys

import glfw

GL_VENDOR = 0x1F00
GL_RENDERER = 0x1F01
GL_VERSION = 0x1F02
GL_EXTENSIONS = 0x1F03
GL_NUM_EXTENSIONS = 0x821D

FUNCTYPE = getattr(ctypes, "WINFUNCTYPE", ctypes.CFUNCTYPE)


def gl_function(name, restype, *argtypes):
    address = glfw.get_proc_address(name)
    if not address:
        raise RuntimeError(f"{name} did not resolve")
    return FUNCTYPE(restype, *argtypes)(address)


def text(value):
    return value.decode(errors="replace") if value else "?"


def on_error(code, description):
    print(f"GLFW error {code:#x}: {text(description) if isinstance(description, bytes) else description}")


def main():
    glfw.set_error_callback(on_error)
    if not glfw.init():
        print("SUMMARY no-context: glfw.init failed")
        return 0
    window = None
    asked = None
    for request in ((4, 6, True), (4, 5, True), (4, 3, True), (3, 3, True), None):
        glfw.default_window_hints()
        glfw.window_hint(glfw.VISIBLE, glfw.FALSE)
        if request is not None:
            glfw.window_hint(glfw.CONTEXT_VERSION_MAJOR, request[0])
            glfw.window_hint(glfw.CONTEXT_VERSION_MINOR, request[1])
            glfw.window_hint(glfw.OPENGL_PROFILE, glfw.OPENGL_CORE_PROFILE)
        window = glfw.create_window(64, 64, "gl-probe", None, None)
        if window:
            asked = "default context" if request is None else f"{request[0]}.{request[1]} core"
            break
    if not window:
        print("SUMMARY no-context: no context at all")
        glfw.terminate()
        return 0
    glfw.make_context_current(window)
    prefix = "" if asked == "4.6 core" else f"NO 4.6, best={asked} | "

    get_string = gl_function("glGetString", ctypes.c_char_p, ctypes.c_uint)
    get_string_i = gl_function("glGetStringi", ctypes.c_char_p, ctypes.c_uint, ctypes.c_uint)
    get_integer = gl_function("glGetIntegerv", None, ctypes.c_uint, ctypes.POINTER(ctypes.c_int))

    count = ctypes.c_int(0)
    get_integer(GL_NUM_EXTENSIONS, ctypes.byref(count))
    extensions = {text(get_string_i(GL_EXTENSIONS, index)) for index in range(count.value)}
    bindless = "GL_ARB_bindless_texture" in extensions

    print(f"vendor:     {text(get_string(GL_VENDOR))}")
    print(f"renderer:   {text(get_string(GL_RENDERER))}")
    print(f"version:    {text(get_string(GL_VERSION))}")
    print(f"extensions: {count.value}")
    for name in ("GL_ARB_bindless_texture", "GL_ARB_gl_spirv", "GL_ARB_indirect_parameters",
                 "GL_ARB_shader_draw_parameters"):
        print(f"  {name}: {'yes' if name in extensions else 'no'}")
    print(f"SUMMARY {prefix}renderer={text(get_string(GL_RENDERER))} | "
          f"version={text(get_string(GL_VERSION))} | bindless={'yes' if bindless else 'no'}")

    glfw.destroy_window(window)
    glfw.terminate()
    return 0


if __name__ == "__main__":
    sys.exit(main())
