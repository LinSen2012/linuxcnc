/********************************************************************
* Description: cpp_blockwrapper.hh
*   C++ Block Wrapper Class for linuxQtCnc
*
*   This file provides a pure C++ replacement for the Python-based
*   block wrapper that was used in LinuxCNC for accessing G-code
*   block information.
*
*   Original Python block features:
*   - block.g_modes - array of active G modes
*   - block.m_modes - array of active M modes
*   - block.params  - array of parameters
*   - block.comment - G-code comment
*   - block.o_name  - O-word name
*
*   C++ Replacement:
*   - block.getGMode(group) - get G mode for group
*   - block.getMMode(group) - get M mode for group
*   - block.getParam(index) - get parameter value
*   - block.getComment()    - get comment string
*   - block.getOName()      - get O-word name
*
*   Derived from a work by Michael Haberler
*
* Author: linuxQtCnc project
* License: GPL Version 2
* System: Linux / Windows
*
* Copyright (c) 2026 All rights reserved.
********************************************************************/

#ifndef CPP_BLOCKWRAPPER_HH
#define CPP_BLOCKWRAPPER_HH

#include "interp_fwd.hh"
#include <string>
#include <vector>

// Forward declarations
struct block_struct;

namespace InterpBlocks {

// ========================================================================
// Block Wrapper Class
// ========================================================================

class BlockWrapper {
public:
    explicit BlockWrapper(block_struct* block);
    BlockWrapper(const BlockWrapper& other);
    BlockWrapper& operator=(const BlockWrapper& other);
    ~BlockWrapper();

    // ---- G Modes Access ----

    // Get G mode for a specific modal group
    int getGMode(int group) const;

    // Get all G modes as a vector
    std::vector<int> getAllGModes() const;

    // Set G mode for a specific group
    void setGMode(int group, int mode);

    // ---- M Modes Access ----

    // Get M mode for a specific modal group
    int getMMode(int group) const;

    // Get all M modes as a vector
    std::vector<int> getAllMModes() const;

    // Set M mode for a specific group
    void setMMode(int group, int mode);

    // ---- Parameters Access ----

    // Get parameter by index
    double getParam(int index) const;

    // Get all parameters as a vector
    std::vector<double> getAllParams() const;

    // Set parameter by index
    void setParam(int index, double value);

    // ---- Word Flags and Values ----

    // Check if a word is present in the block
    bool hasX() const;
    bool hasY() const;
    bool hasZ() const;
    bool hasA() const;
    bool hasB() const;
    bool hasC() const;
    bool hasF() const;
    bool hasS() const;
    bool hasT() const;
    bool hasP() const;
    bool hasQ() const;
    bool hasR() const;
    bool hasI() const;
    bool hasJ() const;
    bool hasK() const;
    bool hasL() const;
    bool hasH() const;
    bool hasD() const;
    bool hasE() const;
    bool hasU() const;
    bool hasV() const;
    bool hasW() const;

    // Get word values
    double getX() const;
    double getY() const;
    double getZ() const;
    double getA() const;
    double getB() const;
    double getC() const;
    double getF() const;
    double getS() const;
    int    getT() const;
    double getP() const;
    double getQ() const;
    double getR() const;
    double getI() const;
    double getJ() const;
    double getK() const;
    int    getL() const;
    int    getH() const;
    double getD() const;
    double getE() const;
    double getU() const;
    double getV() const;
    double getW() const;

    // ---- Comment and O-word ----

    // Get the comment string
    std::string getComment() const;

    // Set the comment string
    void setComment(const std::string& comment);

    // Get the O-word name
    std::string getOName() const;

    // Set the O-word name
    void setOName(const std::string& name);

    // ---- Line Number ----

    // Get the line number
    int getLineNumber() const;

    // Set the line number
    void setLineNumber(int line);

    // ---- Sequence Number ----

    // Get the sequence number (N-word)
    int getSequenceNumber() const;

    // ---- Tool Information ----

    // Get tool number
    int getToolNumber() const;

    // Get pocket number
    int getPocketNumber() const;

    // ---- Motion Type ----

    // Get motion type (G0, G1, G2, G3, etc.)
    int getMotionType() const;

    // ---- Arc Parameters ----

    // Get arc center offsets
    double getArcCenterX() const;  // I
    double getArcCenterY() const;  // J
    double getArcCenterZ() const;  // K

    // Get arc radius (if R format)
    double getArcRadius() const;

    // ---- Cutter Compensation ----

    // Get cutter compensation direction
    int getCutterCompDirection() const;

    // ---- Cycle Parameters ----

    // Get canned cycle retract position
    double getCycleRetract() const;

    // Get canned cycle depth
    double getCycleDepth() const;

    // ---- Raw Block Access ----

    // Get raw block pointer (for advanced use)
    block_struct* rawBlock() { return m_block; }
    const block_struct* rawBlock() const { return m_block; }

private:
    block_struct* m_block;
};

// ========================================================================
// Block Array Wrapper (for nested remaps)
// ========================================================================

class BlockArrayWrapper {
public:
    explicit BlockArrayWrapper(block_struct* blocks, int size);
    ~BlockArrayWrapper();

    // Get block at index
    BlockWrapper getBlock(int index) const;

    // Get array size
    int size() const { return m_size; }

private:
    block_struct* m_blocks;
    int m_size;
};

} // namespace InterpBlocks

#endif // CPP_BLOCKWRAPPER_HH