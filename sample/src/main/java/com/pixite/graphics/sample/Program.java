package com.pixite.graphics.sample;

import android.support.v4.util.ArrayMap;
import android.util.Log;

import static android.opengl.GLES20.GL_COMPILE_STATUS;
import static android.opengl.GLES20.GL_FALSE;
import static android.opengl.GLES20.GL_FRAGMENT_SHADER;
import static android.opengl.GLES20.GL_LINK_STATUS;
import static android.opengl.GLES20.GL_VERTEX_SHADER;
import static android.opengl.GLES20.glAttachShader;
import static android.opengl.GLES20.glCompileShader;
import static android.opengl.GLES20.glCreateProgram;
import static android.opengl.GLES20.glCreateShader;
import static android.opengl.GLES20.glDeleteProgram;
import static android.opengl.GLES20.glDeleteShader;
import static android.opengl.GLES20.glDetachShader;
import static android.opengl.GLES20.glGetAttribLocation;
import static android.opengl.GLES20.glGetProgramiv;
import static android.opengl.GLES20.glGetShaderInfoLog;
import static android.opengl.GLES20.glGetShaderiv;
import static android.opengl.GLES20.glGetUniformLocation;
import static android.opengl.GLES20.glLinkProgram;
import static android.opengl.GLES20.glShaderSource;

final class Program {
    private static final String TAG = Program.class.getSimpleName();

    private static final ArrayMap<String, Program> programs = new ArrayMap<>();

    private String name;
    private int program;
    private int vertexShader;
    private int fragmentShader;
    private final ArrayMap<String, Integer> attributes = new ArrayMap<>();
    private final ArrayMap<String, Integer> uniforms = new ArrayMap<>();

    private Program(String name) {
        this.name = name;
    }

    public static Program get(String name) {
        return programs.get(name);
    }

    public static Program load(String name, String vertexSource, String fragmentSource) {
        Program program = programs.get(name);
        if (program == null) {
            program = new Program(name);
            program.compile(name, vertexSource, fragmentSource);
        }
        return program;
    }
    /**
     * Returns the OpenGL name (int location) of the program.
     * @return The OpenGL name of the program.
     */
    public int getName() {
        return program;
    }

    /**
     * Retrieves a uniform location by name.
     * @param name The name of the uniform in the shader.
     * @return The location of the uniform.
     */
    public int uniformLocation(String name) {
        Integer loc = uniforms.get(name);
        if (loc == null) {
            loc = glGetUniformLocation(program, name);
            if (loc == -1) {
                Log.d(TAG, String.format("Unknown uniform %s", name));
                return -1;
            }
            uniforms.put(name, loc);
        }
        return loc;
    }

    /**
     * Retrieves an attribute location by name.
     * @param name The name of the attribute in the shader.
     * @return The location of the attribute.
     */
    public int attribLocation(String name) {
        Integer loc = attributes.get(name);
        if (loc == null) {
            loc = glGetAttribLocation(program, name);
            if (loc == -1) {
                Log.d(TAG, String.format("Unknown attribute %s", name));
                return -1;
            }
            attributes.put(name, loc);
        }
        return loc;
    }

    private void compile(String name, String vertexSource, String fragmentSource) {
        if ((vertexShader = loadShader(GL_VERTEX_SHADER, vertexSource)) < 0) {
            Log.d(TAG, "Couldn't compile " + name + " vertex shader.");
            return;
        }
        if ((fragmentShader = loadShader(GL_FRAGMENT_SHADER, fragmentSource)) < 0) {
            Log.d(TAG, "Couldn't compile " + name + " fragment shader.");
            glDeleteShader(vertexShader);
            return;
        }
        program = linkProgram(vertexShader, fragmentShader);
    }

    private int linkProgram(int... shaders) {
        int program = glCreateProgram();

        for (int shader : shaders) {
            glAttachShader(program, shader);
        }

        glLinkProgram(program);

        int[] status = new int[1];
        glGetProgramiv(program, GL_LINK_STATUS, status, 0);
        if (status[0] == GL_FALSE) {
            destroy(program, shaders);
            return 0;
        }

        return program;
    }

    private void destroy(int program, int... shaders) {
        for (int shader : shaders) {
            glDetachShader(program, shader);
            glDeleteShader(shader);
        }
        glDeleteProgram(program);
    }

    private int loadShader(int type, String source) {
        int shader = glCreateShader(type);
        glShaderSource(shader, source);
        glCompileShader(shader);

        int[] compiled = new int[1];
        glGetShaderiv(shader, GL_COMPILE_STATUS, compiled, 0);
        if (compiled[0] == 0) {
            Log.d(TAG, String.format("Could not compile shader: %s", glGetShaderInfoLog(shader)));
            glDeleteShader(shader);
            shader = 0;
        }

        return shader;
    }
}
