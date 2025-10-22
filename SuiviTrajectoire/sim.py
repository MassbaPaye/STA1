# https://www-sop.inria.fr/members/Pascal.Morin/PapersDR2/chap_08.pdf

import numpy as np
import matplotlib.pyplot as plt
import threading
import time


v_ref=100
k0=0.05
l1=40


# -----------------------------
# Utility: wrap angle to [-pi, pi]
# -----------------------------
def wrap_angle(a):
    return np.arctan2(np.sin(a), np.cos(a))

# -----------------------------
# Parameterized path interface
# A path is defined by functions x(s), y(s) on s in [s_min, s_max]
# and their derivatives dx/ds, dy/ds when needed.
# Example provided: cubic polynomial parameterized by s (not by x).
# -----------------------------

# ============================
# 1️⃣ ROBOT SIMULATION THREAD
# ============================
class RobotSimulator(threading.Thread):
    def __init__(self, x0=[0.0, 0.0, 0.0], dt=0.001):
        super().__init__()
        self.dt = dt
        self.state = np.array(x0, dtype=float)
        self.v = 0.0
        self.omega = 0.0
        self.running = False
        self.lock = threading.Lock()

    # Setter for commands
    def set_velocity(self, v_cmd, omega_cmd):
        with self.lock:
            self.v = v_cmd
            self.omega = omega_cmd

    # Getters for measured data
    def get_measurements(self):
        with self.lock:
            return self.state.copy()

    # Main loop
    def run(self):
        self.running = True
        while self.running:
            with self.lock:
                x, y, th = self.state
                v, w = self.v, self.omega
            # Discrete unicycle integration
            x += self.dt * v * np.cos(th)
            y += self.dt * v * np.sin(th)
            th += self.dt * w
            with self.lock:
                self.state = np.array([x, y, th])
            time.sleep(self.dt)

    def stop(self):
        self.running = False

# ============================
# 2️⃣ CONTROLLER THREAD (feedback law)
# ============================
class FeedbackController(threading.Thread):
    def __init__(self, robot: RobotSimulator, path_coeffs, v_ref, k0, l1, dt=0.005):
        super().__init__()
        self.robot = robot
        self.path_coeffs = path_coeffs  # [a0, a1, a2, a3]
        self.v_ref = v_ref
        self.k0 = k0
        self.l1 = l1
        self.dt = dt
        self.running = False
        self.history = []

    def polynomial_path(self, x):
        a0, a1, a2, a3 = self.path_coeffs
        y = a0 + a1 * x + a2 * x ** 2 + a3 * x ** 3
        dy_dx = a1 + 2 * a2 * x + 3 * a3 * x ** 2
        theta_ref = np.arctan(dy_dx)
        return y, theta_ref

    def compute_feedback(self, x, y, theta):
        y_ref, theta_ref = self.polynomial_path(x)
        d = -(y - y_ref) * np.cos(theta_ref)  # simple approximation of lateral error
        theta_e = theta - theta_ref
        theta_e = np.arctan2(np.sin(theta_e), np.cos(theta_e))
        omega = -(self.v_ref / self.l1) * np.tan(theta_e) - self.k0 * self.v_ref * d
        return self.v_ref, omega, d, theta_e

    def run(self):
        self.running = True
        while self.running:
            x, y, theta = self.robot.get_measurements()
            v_cmd, omega_cmd, d, theta_e = self.compute_feedback(x, y, theta)
            self.robot.set_velocity(v_cmd, omega_cmd)
            self.history.append((x, y, theta, d, theta_e, v_cmd, omega_cmd))
            time.sleep(self.dt)

    def stop(self):
        self.running = False

# ============================
# 3️⃣ MAIN EXECUTION EXAMPLE
# ============================
if __name__ == "__main__":
    robot = RobotSimulator(x0=[0.0, 0.0, np.deg2rad(180)])
    controller = FeedbackController(robot, path_coeffs=[0, -5e-3, 5e-2, 0.5e-3], v_ref=v_ref, k0=k0, l1=l1)  # cubic path

    robot.start()
    controller.start()

    try:
        time.sleep(5)  # run simulation for 15 seconds
    except KeyboardInterrupt:
        pass

    controller.stop()
    robot.stop()
    robot.join()
    controller.join()

    hist = np.array(controller.history)
    plt.figure(figsize=(10, 5))
    plt.subplot(1, 2, 1)
    plt.plot(hist[:, 0], hist[:, 1], 'r', label='Robot trajectory')

    # Plot reference path
    x_ref = np.linspace(min(hist[:, 0]) - 0.5, max(hist[:, 0]) + 0.5, 200)
    y_ref = controller.path_coeffs[0] + controller.path_coeffs[1]*x_ref + \
             controller.path_coeffs[2]*x_ref**2 + controller.path_coeffs[3]*x_ref**3
    plt.plot(x_ref, y_ref, 'k--', label='Polynomial reference path')
    plt.xlabel('x (mm)')
    plt.ylabel('y (mm)')
    plt.axis('equal')
    plt.legend()
    plt.title('Robot Path Following')

    plt.subplot(1, 2, 2)
    plt.plot(hist[:, 3], label='Lateral error d (mm)')
    plt.plot(np.rad2deg(hist[:, 4]), label='Heading error θe (deg)')
    plt.xlabel('Time step')
    plt.legend()
    plt.grid()
    plt.tight_layout()
    plt.show()
