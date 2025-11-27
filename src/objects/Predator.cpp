//
// Created by moltma on 11/21/25
//

#include "Predator.h"

#include "Prey.h"
#include "Plant.h"
#include "../sim/Simulation.h"

namespace swarmulator {
     Predator::Predator() : NeuralAgent() {
         interaction_radius_ = 25;
     }

     Predator::Predator(const Vector3 position, const Vector3 rotation) : NeuralAgent(position, rotation) {
         interaction_radius_ = 25;
     }

    void Predator::update(Simulation &context, const std::list<SimObject *> &neighborhood, const float dt) {
         NeuralAgent::update(context, neighborhood, dt); // do normal update (think and move, update energy from base and signal cost)

         // if you can reproduce, do it
         if (energy_ >= reproduction_threshold_) {
             energy_ -= reproduction_cost_;
             auto child = *this;
             child.position_ = {
                 randfloat(-context.get_world_size().x / 2.f, context.get_world_size().x / 2.f),
                 randfloat(-context.get_world_size().y / 2.f, context.get_world_size().y / 2.f),
                 randfloat(-context.get_world_size().z / 2.f, context.get_world_size().z / 2.f)
             };
             child.rotation_ = {randfloat(-1, 1), randfloat(-1, 1), randfloat(-1, 1)};
             child.rotation_ = Vector3Normalize(child.rotation_);
             child.mutate();
             child.parent_id_ = id_;
             child.time_born_ = context.get_sim_time();
             child.energy_ = initial_energy_;
             context.add_object(child);
         }

         // if you're near prey, try to kill and eat it
         // the value of the decision output of the neural net (index 4) decides whether to chase prey or not (> 0.5 means give chase)
         // when the predator is chasing, it consumes extra energy
         // the predator can only chase one prey per update
         // prey with full energy have a 50/50 chance of escaping, which decreases linearly with the prey's energy (simulates general fitness)
         // when a predator kills its prey, it receives all of that prey's energy and the prey is removed from the simulation
         if (output_(0, 4) > 0.5f) {
            for (const auto neighbor : neighborhood) {
                if (const auto prey = dynamic_cast<Prey*>(neighbor); prey != nullptr && prey->active()) {
                    // chasing, so consume extra energy
                    energy_ -= chase_cost_ * dt;
                    const float diff = reproduction_threshold_ - prey->get_energy();
                    const float chance = (-0.5f / reproduction_threshold_) * diff + 0.5f; // chance of successfully capturing the prey based on how healthy it is
                    if (randfloat() <= chance) {
                        // eat if you captured
                        energy_ += prey->get_energy(); // gain its energy
                        prey->deactivate(); // kill the prey
                    }
                    break; // only chase one prey per frame
                }
                else if (const auto predator = dynamic_cast<Predator*>(neighbor); predator != nullptr && predator->active()) {
                    // chasing, so consume extra energy
                    energy_ -= predator->get_energy();
                    const float diff = reproduction_threshold_ - predator->get_energy();
                    const float chance = (-0.5f / reproduction_threshold_) * diff + 0.5f;
                    if (randfloat() <= chance) {
                        energy_ += predator->get_energy();
                        predator->deactivate();
                    }
                    break;
                }
            }
         }
     }

}
