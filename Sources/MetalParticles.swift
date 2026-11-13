import AppKit
import MetalKit
import QuartzCore

// Hybrid: this view owns only kind 0/5; SnowView retains all vector artwork/input.
final class ParticleMetalView: MTKView {
    override var isOpaque: Bool { false }
    override var acceptsFirstResponder: Bool { false }
    override func hitTest(_ point: NSPoint) -> NSView? { nil }
}
struct MetalParticle {
    var positionRadiusAlpha: SIMD4<Float>
    var angleKind: SIMD4<Float>
}
final class MetalParticles {
    static let capacity = 9300
    let view: ParticleMetalView
    private let pipeline: MTLRenderPipelineState
    private let queue: MTLCommandQueue
    private let buffer: MTLBuffer
    private let worker = DispatchQueue(label: "DesktopSnow.Metal", qos: .userInteractive)
    private let available = DispatchSemaphore(value: 1)
    private var missingDrawables = 0 // worker-only; transient unavailability is expected
    var failed: ((String) -> Void)?

    init(frame: NSRect) throws {
        guard let device = MTLCreateSystemDefaultDevice(),
              let queue = device.makeCommandQueue(),
              let buffer = device.makeBuffer(length: Self.capacity * MemoryLayout<MetalParticle>.stride, options: .storageModeShared)
        else { throw NSError(domain: "DesktopSnow.Metal", code: 1, userInfo: [NSLocalizedDescriptionKey: "Metal device/queue/buffer unavailable"]) }
        precondition(MemoryLayout<MetalParticle>.stride == 32)
        precondition(MemoryLayout<MetalParticle>.offset(of: \.angleKind) == 16)
        guard let url = Bundle.main.url(forResource: "Particles", withExtension: "metal") else {
            throw NSError(domain: "DesktopSnow.Metal", code: 2, userInfo: [NSLocalizedDescriptionKey: "Missing Particles.metal resource"])
        }
        let library = try device.makeLibrary(source: String(contentsOf: url, encoding: .utf8), options: nil)
        let descriptor = MTLRenderPipelineDescriptor()
        descriptor.vertexFunction = library.makeFunction(name: "snowVertex")
        descriptor.fragmentFunction = library.makeFunction(name: "snowFragment")
        let color = descriptor.colorAttachments[0]!
        color.pixelFormat = .bgra8Unorm
        color.isBlendingEnabled = true
        color.sourceRGBBlendFactor = .one
        color.sourceAlphaBlendFactor = .one
        color.destinationRGBBlendFactor = .oneMinusSourceAlpha
        color.destinationAlphaBlendFactor = .oneMinusSourceAlpha
        pipeline = try device.makeRenderPipelineState(descriptor: descriptor)
        self.queue = queue; self.buffer = buffer
        view = ParticleMetalView(frame: frame, device: device)
        // Apple explicit drawing mode: no second animation timer.
        // https://developer.apple.com/documentation/metalkit/mtkview
        view.isPaused = true; view.enableSetNeedsDisplay = false
        view.colorPixelFormat = .bgra8Unorm; view.framebufferOnly = true
        view.sampleCount = 1; view.depthStencilPixelFormat = .invalid
        view.clearColor = MTLClearColorMake(0, 0, 0, 0)
        view.autoresizingMask = [.width, .height]
        view.autoResizeDrawable = false
        guard let layer = view.layer as? CAMetalLayer else {
            throw NSError(domain: "DesktopSnow.Metal", code: 3, userInfo: [NSLocalizedDescriptionKey: "MTKView has no CAMetalLayer"])
        }
        layer.isOpaque = false; layer.backgroundColor = NSColor.clear.cgColor
        layer.maximumDrawableCount = 2; layer.allowsNextDrawableTimeout = true
        NSLog("DesktopSnow Metal: hybrid particles, device %@", device.name)
    }
    // Called only on main. No waiting on GPU/drawables here; at most one frame in flight.
    // CAMetalLayer drawable acquisition/encoding runs off-main, not NSView operations.
    // https://developer.apple.com/documentation/quartzcore/cametallayer
    func submit(_ sprites: [SnowSprite], world: CGRect, time: Double) {
        guard !view.isHidden, view.window?.isVisible == true,
              let layer = view.layer as? CAMetalLayer,
              world.width > 0, world.height > 0,
              available.wait(timeout: .now()) == .success else { return }
        // One drawable per physical monitor, never a desktop-union/offscreen surface.
        // At most 8192 per side / 16M pixels; extreme displays lower resolution safely.
        let backing = view.window?.backingScaleFactor ?? 1
        let scale = min(backing, 8192 / max(world.width, world.height),
                        sqrt(16_777_216 / (world.width * world.height)))
        let size = CGSize(width: max(1, floor(world.width * scale)), height: max(1, floor(world.height * scale)))
        if view.drawableSize != size { view.drawableSize = size }
        var particles: [MetalParticle] = []
        particles.reserveCapacity(min(sprites.count, Self.capacity))
        for s in sprites where s.kind == 0 || s.kind == 5 {
            guard particles.count < Self.capacity else { break }
            let r = s.r * 1.6
            guard world.intersects(CGRect(x:s.x-r,y:s.y-r,width:2*r,height:2*r)) else { continue }
            particles.append(MetalParticle(positionRadiusAlpha: SIMD4(Float(s.x-world.minX),Float(s.y-world.minY),Float(s.r),Float(s.alpha)),
                angleKind: SIMD4(Float(s.kind == 5 ? s.angle : time*0.35+s.x*0.01),s.kind == 5 ? 1 : 0,0,0)))
        }
        let snapshot = particles
        let viewport = SIMD4<Float>(Float(world.width),Float(world.height),0,0)
        worker.async { [self] in
            // Empty frames still clear the drawable, so Clear/pause cannot leave trails.
            guard let drawable = layer.nextDrawable(), let command = queue.makeCommandBuffer() else {
                missingDrawables += 1
                if missingDrawables == 1 { NSLog("DesktopSnow Metal: drawable temporarily unavailable") }
                if missingDrawables == 30 {
                    DispatchQueue.main.async { self.failed?("30 consecutive drawable/command acquisition failures") }
                }
                available.signal(); return
            }
            missingDrawables = 0
            if !snapshot.isEmpty {
                snapshot.withUnsafeBytes { bytes in
                    buffer.contents().copyMemory(from: bytes.baseAddress!, byteCount: bytes.count)
                }
            }
            let pass = MTLRenderPassDescriptor()
            pass.colorAttachments[0].texture = drawable.texture
            pass.colorAttachments[0].loadAction = .clear
            pass.colorAttachments[0].storeAction = .store
            pass.colorAttachments[0].clearColor = MTLClearColorMake(0,0,0,0)
            guard let encoder = command.makeRenderCommandEncoder(descriptor: pass) else {
                available.signal()
                DispatchQueue.main.async { self.failed?("Cannot create Metal render encoder") }; return
            }
            encoder.setRenderPipelineState(pipeline)
            encoder.setVertexBuffer(buffer, offset: 0, index: 0)
            var uniform = viewport
            encoder.setVertexBytes(&uniform, length: MemoryLayout<SIMD4<Float>>.stride, index: 1)
            if !snapshot.isEmpty {
                encoder.drawPrimitives(type: .triangle, vertexStart: 0, vertexCount: 6, instanceCount: snapshot.count)
            }
            encoder.endEncoding()
            command.present(drawable)
            command.addCompletedHandler { [self] completed in
                available.signal()
                if completed.status == .error {
                    let message = completed.error?.localizedDescription ?? "GPU command failed"
                    DispatchQueue.main.async { self.failed?(message) }
                }
            }
            command.commit()
        }
    }
}
