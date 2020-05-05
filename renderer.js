// This file is required by the index.html file and will
// be executed in the renderer process for that window.
// No Node.js APIs are available in this process because
// `nodeIntegration` is turned off. Use `preload.js` to
// selectively enable features needed in the rendering
// process.

class Renderer {
	constructor() {
        let c = document.getElementById('my_canvas');
		this.canvas = c;
        this.width = c.width; this.height = c.height;
		this.ctx = this.canvas.getContext('2d');
        this.frame_count = 0;
		this.addBindings();
		this.addListeners();
		this.update();
		this.run();
	}

	addBindings() {
		this.update = this.update.bind(this);
        this.run    = this.run.bind(this);
	}

	addListeners() {
		window.addEventListener('resize', this.update);
	}

	update() {
        console.log("update, " + 
                    window.innerWidth + " x " + 
                    window.innerHeight);
        if (false) {
            this.width = window.innerWidth;
            this.height = window.innerHeight;
            this.canvas.width = this.width;
            this.canvas.height = this.height;
        }
	}

	run() {
        draw_timeline(this.ctx);
        window.requestAnimationFrame(this.run);
	}
}

g_renderer = new Renderer();