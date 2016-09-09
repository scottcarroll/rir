#pragma once

namespace rir {

/** A namespace for now, will contain execution characteristics inside analyses.
 */
class Execution {
public:
    /** Execution mode governs how the analysis deals with sideeffects (i.e. variable writes)

      In normal mode, variable writes override existing information, while in maybe mode are merged with existing information as the maybe mode signifies that the xecution may not happen at all. Maybe mode also means that any new bindings will be maybe bindings only.
     */
    enum class Mode {
        normal,
        maybe,
    };

    /** Returns the current execution mode.
     */
    Mode mode() const;

    /** Returns true if the assumptions are to be used in the variable refinement.
     */
    bool useAssumptions() const;



};

} // namespace rir

