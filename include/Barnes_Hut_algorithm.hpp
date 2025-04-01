#ifndef BARNESHUT_HPP
#define BARNESHUT_HPP

#include <iostream>
#include <vector>
#include <unistd.h>
#include "SFML/Graphics.hpp"
#include "vector_operator.hpp"

#define MAX_SIZE 10000
#define THETA 0.5

namespace Barnes_Hut_struct {
    
    struct Node {
        
        // <next> is an array that contains the indices of the 4 children node of this node. 
        // In order : next[0] node 1, next[1] node 2, next[2] node 3, next[4] node 4. 
        // In use Euclidean 2D space node notation. 
        int next[4] = {0, 0, 0, 0};

        // <center> is the geometric center of the node, wich is a square, with width and height <size>*2. 
        sf::Vector2f center;
        float size;

        sf::Vector2f centerOfMass;
        int mass = 0;

    };
    
    struct Quadtree {

        std::vector<Node> qtree;

        void init(){
            
            qtree.clear();
            qtree.insert(qtree.begin() , Node());
            qtree[0].next[0] = 0;
            qtree[0].center = {640, 360};
            qtree[0].size = MAX_SIZE;

        }

        void subdivide(int node){
            
            int qtree_size = qtree.size();
            float new_node_size = qtree[node].size/2;
            
            // node I
            qtree.insert(qtree.end(), Node());
            qtree[qtree_size].center = {(qtree[node].center.x + (qtree[node].size/2)), (qtree[node].center.y - (qtree[node].size/2))};
            qtree[qtree_size].size = new_node_size;
            qtree[node].next[0] = qtree_size;

            // node II
            qtree.insert(qtree.end(), Node());
            qtree[qtree_size +1].center = {(qtree[node].center.x - (qtree[node].size/2)), (qtree[node].center.y - (qtree[node].size/2))};
            qtree[qtree_size +1].size = new_node_size;
            qtree[node].next[1] = qtree_size +1;

            // node III
            qtree.insert(qtree.end(), Node());
            qtree[qtree_size +2].center = {(qtree[node].center.x - (qtree[node].size/2)), (qtree[node].center.y + (qtree[node].size/2))};
            qtree[qtree_size +2].size = new_node_size;
            qtree[node].next[2] = qtree_size +2;

            // node IV
            qtree.insert(qtree.end(), Node());
            qtree[qtree_size +3].center = {(qtree[node].center.x + (qtree[node].size/2)), (qtree[node].center.y + (qtree[node].size/2))};
            qtree[qtree_size +3].size = new_node_size;
            qtree[node].next[3] = qtree_size +3;

        }

        int find_quadrant(sf::Vector2f pos){
            for(int i=0; i<qtree.size(); ++i){
                if(abs(qtree[i].center.x - pos.x) <= qtree[i].size && abs(qtree[i].center.y - pos.y) <= qtree[i].size){
                    if(qtree[i].next[0] == 0){
                        return i;
                    }else{
                        i = qtree[i].next[0] - 1;
                    }
                }
            }
            return -1;
        }
    
        void insert(int mass, sf::Vector2f pos){
            
            int i = 0;
            int j = 0;
            
            while(true){

                i = find_quadrant(pos);

                if(i == -1){
                    // std::cout << "object out of bound, mass: " << mass << " pos: "<< pos.x << " " << pos.y << std::endl;
                    // exit(-1);
                    return;
                }

                if(qtree[i].mass != 0){
                    
                    subdivide(i);
                    j = find_quadrant(qtree[i].centerOfMass);
                    qtree[j].centerOfMass = qtree[i].centerOfMass;
                    qtree[j].mass = qtree[i].mass;
                    qtree[i].centerOfMass = (qtree[i].centerOfMass*qtree[i].mass +pos*mass) / (qtree[i].mass + mass);
                    qtree[i].mass += mass;

                }else{

                    qtree[i].centerOfMass = pos;
                    qtree[i].mass = mass;
                    return;

                }
            }
        }

        sf::Vector2f update_acceleration_(int mass, sf::Vector2f pos, int node){
            
            // If the current node is an external node (and it is not body b), 
            // calculate the force exerted by the current node on b, and add this amount to b’s net force.
            if(qtree[node].next[0] == 0){
                
                if(qtree[node].centerOfMass == pos){
                
                    return {0.f,0.f};

                }else{

                    // sf::Vector2f direction = qtree[node].centerOfMass - pos;
                    // float magnitude_sq = direction.x*direction.x + direction.y*direction.y;
                    float magnitude_sq = (qtree[node].centerOfMass - pos).x*(qtree[node].centerOfMass - pos).x + (qtree[node].centerOfMass - pos).y*(qtree[node].centerOfMass - pos).y;
                    if(magnitude_sq >= 3.f){
                        float magnitude = sqrt(magnitude_sq);
                        return (qtree[node].centerOfMass - pos) * (qtree[node].mass/(magnitude * magnitude_sq));
                    }
                    return {0.f, 0.f};
                }
            }

            // Otherwise, calculate the ratio s/d. (s = width of the node, d = distance between body and center of mass of node)
            // If s/d < θ , treat this internal node as a single body, and calculate the force it exerts on body b, and add this amount to b’s net force.
            // Otherwise, run the procedure recursively on each of the current node’s children.
            if((qtree[node].size*2)/(pos - qtree[node].centerOfMass).length() < THETA){

                float magnitude_sq = (qtree[node].centerOfMass - pos).x*(qtree[node].centerOfMass - pos).x + (qtree[node].centerOfMass - pos).y*(qtree[node].centerOfMass - pos).y;
                if(magnitude_sq >= 3.f){
                    float magnitude = sqrt(magnitude_sq);
                    return (qtree[node].centerOfMass - pos) * (qtree[node].mass/(magnitude * magnitude_sq));
                }
                return {0.f, 0.f};
          
            }else{

                return update_acceleration_(mass, pos, qtree[node].next[0]) + update_acceleration_(mass, pos, qtree[node].next[1]) + update_acceleration_(mass, pos, qtree[node].next[2]) + update_acceleration_(mass, pos, qtree[node].next[3]);
            }

            return {0.f, 0.f};

        }

        sf::Vector2f update_acceleration(int mass, sf::Vector2f pos){

            sf::Vector2f acc = {0.f, 0.f}; 

            for(int i=0; i<qtree.size(); ++i){

                if((qtree[i].size*2)/(pos - qtree[i].centerOfMass).length() < THETA){

                    float magnitude_sq = (qtree[i].centerOfMass - pos).x*(qtree[i].centerOfMass - pos).x + (qtree[i].centerOfMass - pos).y*(qtree[i].centerOfMass - pos).y;
                    float magnitude = sqrt(magnitude_sq);
                    acc += (qtree[i].centerOfMass - pos) * (qtree[i].mass/(magnitude * magnitude_sq));
                
                }else{

                    for(int j = qtree[i].next[0]; j<(qtree[i].next[0]+4); ++j){
                        
                        if((qtree[i].size*2)/(pos - qtree[i].centerOfMass).length() < THETA){

                            float magnitude_sq = (qtree[i].centerOfMass - pos).x*(qtree[i].centerOfMass - pos).x + (qtree[i].centerOfMass - pos).y*(qtree[i].centerOfMass - pos).y;
                            float magnitude = sqrt(magnitude_sq);
                            acc += (qtree[i].centerOfMass - pos) * (qtree[i].mass/(magnitude * magnitude_sq));
                        }
                    }
                }
                
                if(qtree[i].next[0] == 0 && qtree[i].centerOfMass != pos){

                    float magnitude_sq = (qtree[i].centerOfMass - pos).x*(qtree[i].centerOfMass - pos).x + (qtree[i].centerOfMass - pos).y*(qtree[i].centerOfMass - pos).y;
                    float magnitude = sqrt(magnitude_sq);
                    acc += (qtree[i].centerOfMass - pos) * (qtree[i].mass/(magnitude * magnitude_sq));

                }else{

                }

            }

            return acc;
        }


    };
    
}

namespace Burnes_Hut{

    using namespace Barnes_Hut_struct;

    /**
     * @brief 
     */
    void compute_forces(Celestial_body *galaxy){
        
        Quadtree q;
        q.init();

        for(int i=0; i < GALAXY_DIMENSION; ++i){
            q.insert(galaxy[i].mass, galaxy[i].position);
        }

        try{
            for(int i=0; i < GALAXY_DIMENSION; ++i){
                galaxy[i].acceleration = q.update_acceleration_(galaxy[i].mass, galaxy[i].position, 0);
            }
        }
        catch(std::exception e){

            std::cout << e.what() << std::endl;
        }

    }
}

#endif // BARNESHUT_HPP