#include <GL/freeglut.h>
#include <vector>
#include <queue>
#include <limits>
#include <cmath>
#include <SOIL/SOIL.h>

// Forward declarations
void mouse(int button, int state, int x, int y);
void mouseMotion(int x, int y);
void keyboard(unsigned char key, int x, int y);
void idle();
void display();

GLuint bgTexture;
GLuint nodeTexture;
float glowIntensity = 0.0f;
bool glowing = true;

struct Node {
    float x, y;
    bool obstacle = false;
    bool visited = false;
    float globalDistance;
    Node* parent = nullptr;
    std::vector<Node*> neighbors;

    Node(float x_, float y_) : 
        x(x_), y(y_),
        globalDistance(std::numeric_limits<float>::max()) {}
};

std::vector<Node> nodes;
Node* startNode = nullptr;
Node* endNode = nullptr;
Node* draggedNode = nullptr;
float rotationAngle = 0.0f;
bool autoRotate = true;
bool showDebug = false;
bool showHelp = false;

void loadTextures() {
    bgTexture = SOIL_load_OGL_texture(
        "back.png",
        SOIL_LOAD_AUTO,
        SOIL_CREATE_NEW_ID,
        SOIL_FLAG_MIPMAPS | SOIL_FLAG_INVERT_Y
    );
    
    nodeTexture = SOIL_load_OGL_texture(
        "glow.png",
        SOIL_LOAD_AUTO,
        SOIL_CREATE_NEW_ID,
        SOIL_FLAG_MULTIPLY_ALPHA
    );
}

void drawBackground() {
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, bgTexture);
    glBegin(GL_QUADS);
    glTexCoord2f(0, 0); glVertex2f(-1, -1);
    glTexCoord2f(1, 0); glVertex2f(1, -1);
    glTexCoord2f(1, 1); glVertex2f(1, 1);
    glTexCoord2f(0, 1); glVertex2f(-1, 1);
    glEnd();
    glDisable(GL_TEXTURE_2D);
}

void drawNodeGlow(float x, float y, float r, float g, float b) {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, nodeTexture);
    
    float size = 0.1f + fabs(sin(glutGet(GLUT_ELAPSED_TIME)*0.005f));
    glColor4f(r, g, b, glowIntensity * 0.5f);
    glBegin(GL_QUADS);
    glTexCoord2f(0, 0); glVertex2f(x - size, y - size);
    glTexCoord2f(1, 0); glVertex2f(x + size, y - size);
    glTexCoord2f(1, 1); glVertex2f(x + size, y + size);
    glTexCoord2f(0, 1); glVertex2f(x - size, y + size);
    glEnd();
    
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_BLEND);
}

void drawHelpText() {
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0, 800, 0, 600);
    
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glColor3f(1, 1, 1);
    glRasterPos2i(10, 580);
    const char* helpStr = 
        "Controls:\n"
        "Left Click - Set Start (Green)\n"
        "Right Click - Set End (Blue)\n"
        "Middle Click - Toggle Obstacle\n"
        "Space - Toggle Rotation\n"
        "R - Reset\n"
        "H - Toggle Help";
    glutBitmapString(GLUT_BITMAP_9_BY_15, (const unsigned char*)helpStr);

    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
}

void initializeNodes() {
    const int NUM_NODES = 12;
    const float RADIUS = 0.6f;
    nodes.clear();
    
    for(int i = 0; i < NUM_NODES; ++i) {
        float angle = i * (2 * 3.14159f / NUM_NODES);
        float varRadius = RADIUS * (0.7f + 0.3f * sin(angle * 3));
        nodes.emplace_back(
            varRadius * cos(angle),
            varRadius * sin(angle)
        );
    }

    for(int i = 0; i < NUM_NODES; ++i) {
        for(int j = 1; j <= 3; ++j) {
            if(i % j == 0) {
                nodes[i].neighbors.push_back(&nodes[(i + j) % NUM_NODES]);
            }
        }
    }
}

void solveDijkstra() {
    for(auto& node : nodes) {
        node.globalDistance = std::numeric_limits<float>::max();
        node.visited = false;
        node.parent = nullptr;
    }

    if(!startNode) return;
    
    startNode->globalDistance = 0.0f;
    std::priority_queue<std::pair<float, Node*>> pq;
    pq.push({0.0f, startNode});

    while(!pq.empty()) {
        Node* current = pq.top().second;
        pq.pop();
        
        if(current->visited || current->obstacle) continue;
        current->visited = true;

        for(Node* neighbor : current->neighbors) {
            if(neighbor->obstacle) continue;
            
            float dx = current->x - neighbor->x;
            float dy = current->y - neighbor->y;
            float distance = sqrt(dx*dx + dy*dy);
            
            if(!neighbor->visited && (current->globalDistance + distance < neighbor->globalDistance)) {
                neighbor->parent = current;
                neighbor->globalDistance = current->globalDistance + distance;
                pq.push({-neighbor->globalDistance, neighbor});
            }
        }
    }
}

void display() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    drawBackground();
    
    glPushMatrix();
    glRotatef(rotationAngle, 0, 0, 1);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glBegin(GL_LINES);
    for(const auto& node : nodes) {
        for(const auto& neighbor : node.neighbors) {
            float hue = fmod(rotationAngle/360.0f, 1.0f);
            glColor4f(fabs(sin(hue*3.14159f)), fabs(cos(hue*3.14159f)), 0.5f, 0.7f);
            glVertex2f(node.x, node.y);
            glVertex2f(neighbor->x, neighbor->y);
        }
    }
    glEnd();
    glDisable(GL_BLEND);

    for(const auto& node : nodes) {
        if(node.obstacle) {
            drawNodeGlow(node.x, node.y, 0.3f, 0.3f, 0.3f);
        } else {
            if(&node == startNode) drawNodeGlow(node.x, node.y, 0.0f, 1.0f, 0.0f);
            if(&node == endNode) drawNodeGlow(node.x, node.y, 0.0f, 0.0f, 1.0f);
            if(node.visited) drawNodeGlow(node.x, node.y, 1.0f, 0.0f, 0.0f);
        }
    }

    if(endNode && endNode->parent) {
        glColor3f(1.0f, 1.0f, 0.0f);
        glLineWidth(2.0f);
        glBegin(GL_LINE_STRIP);
        Node* current = endNode;
        while(current) {
            glVertex2f(current->x, current->y);
            current = current->parent;
        }
        glEnd();
        glLineWidth(1.0f);
    }

    glPopMatrix();
    
    if(showHelp) drawHelpText();
    
    if(glowing) {
        glowIntensity = 0.5f + 0.5f * sin(glutGet(GLUT_ELAPSED_TIME) * 0.005f);
    }

    glutSwapBuffers();
}

void mouse(int button, int state, int x, int y) {
    float mx = (x/400.0f)*2 - 1;
    float my = 1 - (y/400.0f)*2;
    
    float cosTheta = cos(-rotationAngle * 3.14159f/180.0f);
    float sinTheta = sin(-rotationAngle * 3.14159f/180.0f);
    float rotX = mx*cosTheta - my*sinTheta;
    float rotY = mx*sinTheta + my*cosTheta;

    if(state == GLUT_DOWN) {
        for(auto& node : nodes) {
            float dx = rotX - node.x;
            float dy = rotY - node.y;
            if(dx*dx + dy*dy < 0.0009f) {
                if(button == GLUT_LEFT_BUTTON) {
                    startNode = &node;
                } else if(button == GLUT_RIGHT_BUTTON) {
                    endNode = &node;
                } else if(button == GLUT_MIDDLE_BUTTON) {
                    node.obstacle = !node.obstacle;
                }
                solveDijkstra();
                glutPostRedisplay();
                break;
            }
        }
    }
}

void mouseMotion(int x, int y) {
    if(draggedNode) {
        float mx = (x/400.0f)*2 - 1;
        float my = 1 - (y/400.0f)*2;
        
        float cosTheta = cos(-rotationAngle * 3.14159f/180.0f);
        float sinTheta = sin(-rotationAngle * 3.14159f/180.0f);
        draggedNode->x = mx*cosTheta - my*sinTheta;
        draggedNode->y = mx*sinTheta + my*cosTheta;
        
        solveDijkstra();
        glutPostRedisplay();
    }
}

void keyboard(unsigned char key, int x, int y) {
    switch(key) {
        case ' ':
            autoRotate = !autoRotate;
            break;
        case 'r':
            initializeNodes();
            startNode = endNode = nullptr;
            break;
        case 'd':
            showDebug = !showDebug;
            break;
        case 'h':
            showHelp = !showHelp;
            break;
        case '+':
            rotationAngle += 5.0f;
            break;
        case '-':
            rotationAngle -= 5.0f;
            break;
    }
    glutPostRedisplay();
}

void idle() {
    if(autoRotate) {
        rotationAngle += 0.3f;
        if(rotationAngle >= 360.0f) rotationAngle -= 360.0f;
        glutPostRedisplay();
    }
}

int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitWindowSize(800, 800);
    glutCreateWindow("Art Gallery");
    glutSetOption(GLUT_MULTISAMPLE, 8);
    glEnable(GL_MULTISAMPLE);
    
    loadTextures();
    initializeNodes();
    
    glutDisplayFunc(display);
    glutMouseFunc(mouse);
    glutMotionFunc(mouseMotion);
    glutKeyboardFunc(keyboard);
    glutIdleFunc(idle);
    
    glutMainLoop();
    return 0;
}
