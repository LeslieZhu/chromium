// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Helper functions for GL.

#ifndef GPU_COMMAND_BUFFER_TESTS_GL_TEST_UTILS_H_
#define GPU_COMMAND_BUFFER_TESTS_GL_TEST_UTILS_H_

#include <GLES2/gl2.h>
#include "base/basictypes.h"

class GLTestHelper {
 public:
  static bool HasExtension(const char* extension);
  static bool CheckGLError(const char* msg, int line);

  // Compiles a shader.
  // Returns shader, 0 on failure..
  static GLuint LoadShader(GLenum type, const char* shaderSrc);

  // Attaches 2 shaders and links them to a program.
  // Returns program, 0 on failure..
  static GLuint SetupProgram(GLuint vertex_shader, GLuint fragment_shader);

  // Compiles 2 shaders, attaches and links them to a program
  // Returns program, 0 on failure.
  static GLuint LoadProgram(
      const char* vertex_shader_source,
      const char* fragment_shader_source);

  // Make a unit quad with position only.
  // Returns the created buffer.
  static GLuint SetupUnitQuad(GLint position_location);

  // Checks an area of pixels for a color.
  static bool CheckPixels(
      GLint x, GLint y, GLsizei width, GLsizei height, GLint tolerance,
      uint8* color);

  // Uses ReadPixels to save an area of the current FBO/Backbuffer.
  static bool SaveBackbufferAsBMP(const char* filename, int width, int height);

  // Run unit tests.
  static int RunTests(int argc, char** argv);
};

#endif  // GPU_COMMAND_BUFFER_TESTS_GL_TEST_UTILS_H_
