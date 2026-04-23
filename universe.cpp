struct Vector2D {
    float x, y;
};

class Ant {
public:
    Vector2D position;
    float heading;

    void update(float dt) {

    }
}

class World {
public: 
    std::vector<Ant> ants;

    void step(float dt) {
        for(auto& ant : ants) {
            ant.update(dt);
        }
    }
}
