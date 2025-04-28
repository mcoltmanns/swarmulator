//
// Created by moltmanns on 4/28/25.
//

#include "PDAgent.h"

#include "raymath.h"

namespace swarmulator::agent {
    std::shared_ptr<Agent> PDAgent::update(const std::vector<std::shared_ptr<Agent> > &neighborhood, const std::list<std::shared_ptr<env::Sphere> > &object, const float dt) {
        NeuralAgent::update(neighborhood, object, dt); // perform your normal update

        // set your team according to what the network decided
        team = std::round(output_(0, 4));

        // update your energy according to your neighbors
        float coop_pay = 0;
        float defect_pay = 0;
        for (const auto& n_a: neighborhood) {
            const auto neighbor = dynamic_cast<PDAgent*>(n_a.get());
            if (neighbor == nullptr || neighbor == this) {
                continue;
            }
            auto dist = Vector3DistanceSqr(position_, neighbor->position_);
            if (dist < sense_radius_) {
                float factor = 1.f / (1.f + std::sqrt(dist));
                // if the neighbor is a cooperator, you get bonuses for cooperating and defecting
                if (neighbor->team == 0) {
                    coop_pay += coop_payoff * factor;
                    defect_pay += defect_payoff * factor;
                }
                // if they are a defector, the cooperation bonus drops
                else {
                    coop_pay -= coop_payoff * factor;
                }
            }
        }
        coop_pay *= dt;
        defect_pay *= dt;

        // update energy based on coop/defect choice and assoc. payoffs
        if (team == 0) {
            energy_ += coop_pay;
        }
        else {
            energy_ += defect_pay;
        }

        // if you have the energy, reproduce
        if (energy_ > reproduction_threshold_) {
            energy_ -= reproduction_cost_;
            auto n_a = std::make_shared<PDAgent>(*this);
            n_a->set_energy(energy_);
            n_a->set_time_born(GetTime());
            n_a->mutate();
            return n_a;
        }
        return nullptr;
    }

    void PDAgent::to_ssbo(SSBOAgent *out) const {
        NeuralAgent::to_ssbo(out);
        out->info = Vector2{static_cast<float>(team), 0};
    }
} // swarmulator