#ifndef RAGE_AUTOMATIC_TRANSMISSION_H
#define RAGE_AUTOMATIC_TRANSMISSION_H

struct CarModelAsset;
struct GameCarSpec;

/* The native port can drive every car through the retail automatic gearbox.
 * MT-only assets with missing tuning data receive deterministic host values. */
int RageAutomaticTransmissionSelectable(const struct CarModelAsset *asset);
struct GameCarSpec *RageAutomaticTransmissionSpec(
    struct GameCarSpec *source, int automaticSelected,
    const struct CarModelAsset *model);

#endif
