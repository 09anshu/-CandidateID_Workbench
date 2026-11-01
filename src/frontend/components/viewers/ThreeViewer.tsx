"use client";

import React, { useEffect, useRef, useState } from "react";
import * as THREE from "three";
import { Maximize2, RotateCw, ZoomIn, ZoomOut, Eye } from "lucide-react";

interface StlTriangle {
  normal: [number, number, number];
  v1: [number, number, number];
  v2: [number, number, number];
  v3: [number, number, number];
}

interface TemperatureNode {
  id: number;
  x: number;
  y: number;
  z: number;
  temperature: number;
}

interface ThreeViewerProps {
  stlData?: {
    triangles: StlTriangle[];
    bounds: { min: [number, number, number]; max: [number, number, number] };
    name: string;
  } | null;
  pointCloudData?: {
    nodes: TemperatureNode[];
    temp_min: number;
    temp_max: number;
    bounds: { min: [number, number, number]; max: [number, number, number] };
  } | null;
}

export default function ThreeViewer({ stlData, pointCloudData }: ThreeViewerProps) {
  const mountRef = useRef<HTMLDivElement>(null);
  const sceneRef = useRef<THREE.Scene | null>(null);
  const cameraRef = useRef<THREE.PerspectiveCamera | null>(null);
  const rendererRef = useRef<THREE.WebGLRenderer | null>(null);
  const groupRef = useRef<THREE.Group | null>(null);

  const [selectedNode, setSelectedNode] = useState<TemperatureNode | null>(null);
  const [wireframe, setWireframe] = useState(false);

  // Interaction state
  const isDraggingRef = useRef(false);
  const dragButtonRef = useRef(0); // 0 = left, 2 = right
  const prevMouseRef = useRef({ x: 0, y: 0 });
  const sphericalRef = useRef({ radius: 3.0, theta: Math.PI / 4, phi: Math.PI / 3 });
  const targetRef = useRef(new THREE.Vector3(0, 0, 0));

  // Update camera position from spherical coordinates
  const updateCamera = () => {
    if (!cameraRef.current) return;
    const { radius, theta, phi } = sphericalRef.current;
    const sinPhi = Math.sin(phi);
    const x = targetRef.current.x + radius * sinPhi * Math.sin(theta);
    const y = targetRef.current.y + radius * Math.cos(phi);
    const z = targetRef.current.z + radius * sinPhi * Math.cos(theta);

    cameraRef.current.position.set(x, y, z);
    cameraRef.current.lookAt(targetRef.current);
  };

  // Fit to view
  const fitToView = () => {
    if (!groupRef.current) return;
    const box = new THREE.Box3().setFromObject(groupRef.current);
    if (box.isEmpty()) return;

    const center = new THREE.Vector3();
    box.getCenter(center);
    targetRef.current.copy(center);

    const size = new THREE.Vector3();
    box.getSize(size);
    const maxDim = Math.max(size.x, size.y, size.z, 0.5);

    sphericalRef.current.radius = maxDim * 2.2;
    sphericalRef.current.theta = Math.PI / 4;
    sphericalRef.current.phi = Math.PI / 3;
    updateCamera();
  };

  // Setup Three.js scene
  useEffect(() => {
    const container = mountRef.current;
    if (!container) return;

    const width = container.clientWidth;
    const height = container.clientHeight;

    const scene = new THREE.Scene();
    scene.background = new THREE.Color(0x0a0f1d);
    sceneRef.current = scene;

    const camera = new THREE.PerspectiveCamera(45, width / height, 0.01, 1000);
    cameraRef.current = camera;

    const renderer = new THREE.WebGLRenderer({ antialias: true, alpha: true });
    renderer.setSize(width, height);
    renderer.setPixelRatio(window.devicePixelRatio);
    rendererRef.current = renderer;
    container.appendChild(renderer.domElement);

    // Lights
    const ambientLight = new THREE.AmbientLight(0xffffff, 0.7);
    scene.add(ambientLight);

    const dirLight1 = new THREE.DirectionalLight(0xffffff, 0.8);
    dirLight1.position.set(5, 10, 7);
    scene.add(dirLight1);

    const dirLight2 = new THREE.DirectionalLight(0x38bdf8, 0.4);
    dirLight2.position.set(-5, -5, -5);
    scene.add(dirLight2);

    // Grid helper
    const grid = new THREE.GridHelper(10, 20, 0x1e293b, 0x0f172a);
    grid.position.y = -0.01;
    scene.add(grid);

    // Axes helper
    const axes = new THREE.AxesHelper(0.5);
    scene.add(axes);

    const group = new THREE.Group();
    scene.add(group);
    groupRef.current = group;

    updateCamera();

    // Mouse Controls
    const onMouseDown = (e: MouseEvent) => {
      isDraggingRef.current = true;
      dragButtonRef.current = e.button;
      prevMouseRef.current = { x: e.clientX, y: e.clientY };
    };

    const onMouseMove = (e: MouseEvent) => {
      if (!isDraggingRef.current) return;
      const dx = e.clientX - prevMouseRef.current.x;
      const dy = e.clientY - prevMouseRef.current.y;
      prevMouseRef.current = { x: e.clientX, y: e.clientY };

      if (dragButtonRef.current === 0) {
        // Left click: Rotate
        sphericalRef.current.theta -= dx * 0.01;
        sphericalRef.current.phi = Math.max(0.05, Math.min(Math.PI - 0.05, sphericalRef.current.phi - dy * 0.01));
      } else if (dragButtonRef.current === 2) {
        // Right click: Pan
        const panSpeed = sphericalRef.current.radius * 0.001;
        const forward = new THREE.Vector3();
        camera.getWorldDirection(forward);
        const right = new THREE.Vector3().crossVectors(forward, camera.up).normalize();
        const up = new THREE.Vector3().crossVectors(right, forward).normalize();

        targetRef.current.addScaledVector(right, -dx * panSpeed);
        targetRef.current.addScaledVector(up, dy * panSpeed);
      }
      updateCamera();
    };

    const onMouseUp = () => {
      isDraggingRef.current = false;
    };

    const onWheel = (e: WheelEvent) => {
      e.preventDefault();
      const zoomFactor = e.deltaY > 0 ? 1.1 : 0.9;
      sphericalRef.current.radius = Math.max(0.1, Math.min(50, sphericalRef.current.radius * zoomFactor));
      updateCamera();
    };

    const onContextMenu = (e: MouseEvent) => e.preventDefault();

    const dom = renderer.domElement;
    dom.addEventListener("mousedown", onMouseDown);
    window.addEventListener("mousemove", onMouseMove);
    window.addEventListener("mouseup", onMouseUp);
    dom.addEventListener("wheel", onWheel, { passive: false });
    dom.addEventListener("contextmenu", onContextMenu);

    // Animation loop
    let animId: number;
    const animate = () => {
      animId = requestAnimationFrame(animate);
      renderer.render(scene, camera);
    };
    animate();

    const handleResize = () => {
      if (!container) return;
      const w = container.clientWidth;
      const h = container.clientHeight;
      camera.aspect = w / h;
      camera.updateProjectionMatrix();
      renderer.setSize(w, h);
    };
    window.addEventListener("resize", handleResize);

    return () => {
      cancelAnimationFrame(animId);
      window.removeEventListener("resize", handleResize);
      window.removeEventListener("mousemove", onMouseMove);
      window.removeEventListener("mouseup", onMouseUp);
      dom.removeEventListener("mousedown", onMouseDown);
      dom.removeEventListener("wheel", onWheel);
      dom.removeEventListener("contextmenu", onContextMenu);
      if (dom.parentElement) {
        dom.parentElement.removeChild(dom);
      }
      renderer.dispose();
    };
  }, []);

  // Update Geometry / Point Cloud Objects
  useEffect(() => {
    const group = groupRef.current;
    if (!group) return;

    // Clear previous children
    while (group.children.length > 0) {
      const obj = group.children[0];
      group.remove(obj);
      if ((obj as any).geometry) (obj as any).geometry.dispose();
      if ((obj as any).material) {
        if (Array.isArray((obj as any).material)) {
          (obj as any).material.forEach((m: any) => m.dispose());
        } else {
          (obj as any).material.dispose();
        }
      }
    }

    // 1. Build STL Mesh
    if (stlData && stlData.triangles.length > 0) {
      const positions: number[] = [];
      const normals: number[] = [];

      for (const t of stlData.triangles) {
        positions.push(...t.v1, ...t.v2, ...t.v3);
        normals.push(...t.normal, ...t.normal, ...t.normal);
      }

      const geom = new THREE.BufferGeometry();
      geom.setAttribute("position", new THREE.Float32BufferAttribute(positions, 3));
      geom.setAttribute("normal", new THREE.Float32BufferAttribute(normals, 3));

      const mat = new THREE.MeshStandardMaterial({
        color: 0x3b82f6,
        roughness: 0.35,
        metalness: 0.25,
        wireframe: wireframe,
        side: THREE.DoubleSide,
      });

      const mesh = new THREE.Mesh(geom, mat);
      group.add(mesh);
    }

    // 2. Build Temperature Point Cloud
    if (pointCloudData && pointCloudData.nodes.length > 0) {
      const positions: number[] = [];
      const colors: number[] = [];
      const { temp_min, temp_max, nodes } = pointCloudData;
      const tRange = Math.max(temp_max - temp_min, 1e-4);

      // Jet / Rainbow colormap: Blue -> Cyan -> Green -> Yellow -> Red
      const tempToColor = (t: number) => {
        const u = Math.max(0, Math.min(1, (t - temp_min) / tRange));
        const color = new THREE.Color();
        color.setHSL((1.0 - u) * 0.67, 1.0, 0.5); // 0.67 is blue, 0.0 is red
        return color;
      };

      for (const node of nodes) {
        positions.push(node.x, node.y, node.z);
        const c = tempToColor(node.temperature);
        colors.push(c.r, c.g, c.b);
      }

      const pointGeom = new THREE.BufferGeometry();
      pointGeom.setAttribute("position", new THREE.Float32BufferAttribute(positions, 3));
      pointGeom.setAttribute("color", new THREE.Float32BufferAttribute(colors, 3));

      const pointMat = new THREE.PointsMaterial({
        size: 0.035,
        vertexColors: true,
      });

      const points = new THREE.Points(pointGeom, pointMat);
      group.add(points);
    }

    fitToView();
  }, [stlData, pointCloudData, wireframe]);

  return (
    <div className="relative w-full h-[520px] bg-slate-950 rounded-xl overflow-hidden border border-slate-800 shadow-2xl">
      {/* 3D Canvas Mount */}
      <div ref={mountRef} className="w-full h-full cursor-grab active:cursor-grabbing" />

      {/* Floating Toolbar */}
      <div className="absolute top-4 left-4 flex items-center space-x-2 bg-slate-900/90 backdrop-blur-md px-3 py-1.5 rounded-lg border border-slate-700 text-xs text-slate-200">
        <button
          onClick={fitToView}
          className="flex items-center space-x-1 hover:text-blue-400 transition"
          title="Fit view to geometry bounds"
        >
          <Maximize2 className="w-3.5 h-3.5" />
          <span>Fit View</span>
        </button>
        <span className="text-slate-600">|</span>
        <button
          onClick={() => setWireframe(!wireframe)}
          className={`flex items-center space-x-1 hover:text-blue-400 transition ${wireframe ? "text-blue-400" : ""}`}
          title="Toggle wireframe display"
        >
          <Eye className="w-3.5 h-3.5" />
          <span>Wireframe</span>
        </button>
      </div>

      {/* Camera Instructions Overlay */}
      <div className="absolute bottom-4 left-4 bg-slate-900/80 backdrop-blur-sm px-2.5 py-1 rounded text-[11px] text-slate-400 border border-slate-800">
        <span>Left-click: Rotate &bull; Right-click: Pan &bull; Scroll: Zoom</span>
      </div>

      {/* Temperature Colormap Legend */}
      {pointCloudData && (
        <div className="absolute bottom-4 right-4 bg-slate-900/95 backdrop-blur-md p-3 rounded-lg border border-slate-700 shadow-lg text-xs">
          <div className="font-semibold text-slate-200 mb-1.5 flex justify-between">
            <span>Temperature</span>
            <span className="text-blue-400 font-mono">°C</span>
          </div>
          <div className="flex items-center space-x-2">
            <span className="font-mono text-slate-300">{pointCloudData.temp_max.toFixed(1)}°</span>
            <div
              className="w-36 h-3 rounded"
              style={{
                background: "linear-gradient(to right, #ef4444, #eab308, #22c55e, #06b6d4, #3b82f6)",
              }}
            />
            <span className="font-mono text-slate-300">{pointCloudData.temp_min.toFixed(1)}°</span>
          </div>
          <div className="mt-1 flex justify-between text-[10px] text-slate-400">
            <span>Peak</span>
            <span>Ambient</span>
          </div>
        </div>
      )}
    </div>
  );
}

