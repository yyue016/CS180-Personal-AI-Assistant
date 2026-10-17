(() => {
  const STORAGE_KEY = "paa-prototype-v3";

  const CATALOG = [
    { id: "qwen3.5:9b", size: "6.6 GB" },
    { id: "llama3", size: "4.7 GB" },
    { id: "qwen2.5", size: "4.7 GB" },
    { id: "qwen2.5:3b", size: "1.9 GB" },
  ];

  const DEMO_NOTE = {
    name: "User Guide.md",
    text:
      "personal assistants. Retrieval-augmented generation grounds answers in approved local files. Profile preferences control tone, length, and language without re-prompting. A floating desktop widget keeps chat available while you work on notes and other apps.",
  };

  const $ = (id) => document.getElementById(id);

  const els = {
    setup: $("view-setup"),
    setupForm: $("setupForm"),
    setupName: $("setupName"),
    setupLanguage: $("setupLanguage"),
    setupTone: $("setupTone"),
    setupLength: $("setupLength"),
    setupExamples: $("setupExamples"),
    setupError: $("setupError"),
    launcher: $("launcher"),
    widget: $("widget"),
    widgetDrag: $("widgetDrag"),
    activeModelLabel: $("activeModelLabel"),
    btnSettings: $("btnSettings"),
    btnMinimize: $("btnMinimize"),
    closeSettings: $("closeSettings"),
    paneChat: $("pane-chat"),
    paneSettings: $("pane-settings"),
    chat: $("chat"),
    chatForm: $("chatForm"),
    userInput: $("userInput"),
    chatStatus: $("chatStatus"),
    docInput: $("docInput"),
    docBrowse: $("docBrowse"),
    docChosen: $("docChosen"),
    docError: $("docError"),
    docOk: $("docOk"),
    docList: $("docList"),
    clearDocs: $("clearDocs"),
    modelList: $("modelList"),
    modelError: $("modelError"),
    modelOk: $("modelOk"),
    modelStatus: $("modelStatus"),
    profileForm: $("profileForm"),
    profileName: $("profileName"),
    profileLanguage: $("profileLanguage"),
    profileTone: $("profileTone"),
    profileLength: $("profileLength"),
    profileExamples: $("profileExamples"),
    profileError: $("profileError"),
    profileOk: $("profileOk"),
    notesFolder: $("notesFolder"),
    summaryFreq: $("summaryFreq"),
    enableSchedule: $("enableSchedule"),
    runOnce: $("runOnce"),
    disableSchedule: $("disableSchedule"),
    sumError: $("sumError"),
    sumOk: $("sumOk"),
    sumStatus: $("sumStatus"),
    sumOutput: $("sumOutput"),
    playDemo: $("playDemo"),
    resetDemo: $("resetDemo"),
    demoCaption: $("demoCaption"),
    clock: $("clock"),
  };

  let state = {
    profileSet: false,
    profile: {
      name: "",
      language: "English",
      tone: "Friendly",
      length: "Medium",
      examples: "",
    },
    activeModel: null,
    installed: ["qwen3.5:9b", "llama3"],
    docs: [],
    indexed: false,
    schedule: { enabled: false, folder: "", frequency: "daily" },
  };

  let demoRunning = false;
  let demoTimer = 0;

  function load() {
    try {
      const raw = localStorage.getItem(STORAGE_KEY);
      if (!raw) return;
      const data = JSON.parse(raw);
      state = {
        ...state,
        ...data,
        profile: { ...state.profile, ...(data.profile || {}) },
        schedule: { ...state.schedule, ...(data.schedule || {}) },
      };
      state.profile.language = "English";
    } catch (_) {}
  }

  function save() {
    localStorage.setItem(STORAGE_KEY, JSON.stringify(state));
  }

  function setMsg(el, text) {
    if (!el) return;
    if (!text) {
      el.hidden = true;
      el.textContent = "";
      return;
    }
    el.hidden = false;
    el.textContent = text;
  }

  function wait(ms) {
    return new Promise((r) => {
      demoTimer = setTimeout(r, ms);
    });
  }

  function tickClock() {
    const d = new Date();
    els.clock.textContent = d.toLocaleTimeString([], {
      hour: "2-digit",
      minute: "2-digit",
    });
  }

  /** Open Personal AI Assistant → profile gate → floating widget */
  function route() {
    if (!state.profileSet) {
      els.setup.hidden = false;
      els.widget.hidden = true;
      els.launcher.hidden = true;
      return;
    }
    els.setup.hidden = true;
    openWidget();
    fillProfileForm();
    updateActiveModelLabel();
    renderDocs();
    renderModels();
    renderSchedule();
    if (!els.chat.dataset.booted) {
      els.chat.dataset.booted = "1";
      addMsg(
        "bot",
        `Hi${state.profile.name ? ` ${state.profile.name}` : ""} — I'm your floating assistant. Ask about approved docs anytime. Use ⚙ only for profile, files, or model.`
      );
    }
  }

  function openWidget() {
    els.launcher.hidden = true;
    els.widget.hidden = false;
    showChatPane();
  }

  function minimizeWidget() {
    els.widget.hidden = true;
    els.launcher.hidden = false;
    els.demoCaption.textContent = "Minimized to bubble — typical idle state on the desktop.";
  }

  function showChatPane() {
    els.paneChat.hidden = false;
    els.paneSettings.hidden = true;
  }

  function showSettings(tab = "docs") {
    els.paneChat.hidden = true;
    els.paneSettings.hidden = false;
    document.querySelectorAll(".stab[data-tab]").forEach((b) => {
      b.classList.toggle("active", b.dataset.tab === tab);
    });
    ["docs", "model", "profile", "summaries"].forEach((id) => {
      $(`tab-${id}`).hidden = id !== tab;
    });
  }

  function validateProfile(name, examples) {
    if (!name.trim()) return "Display name is required.";
    if (name.trim().length < 2) return "Display name must be at least 2 characters.";
    if (examples && examples.length > 500) return "Examples must be 500 characters or fewer.";
    return null;
  }

  function applyProfileFields(name, language, tone, length, examples) {
    const err = validateProfile(name, examples);
    if (err) return err;
    state.profile = {
      name: name.trim(),
      language,
      tone,
      length,
      examples: (examples || "").trim(),
    };
    state.profileSet = true;
    save();
    return null;
  }

  els.setupForm.addEventListener("submit", (e) => {
    e.preventDefault();
    const err = applyProfileFields(
      els.setupName.value,
      els.setupLanguage.value,
      els.setupTone.value,
      els.setupLength.value,
      els.setupExamples.value
    );
    if (err) {
      setMsg(els.setupError, err);
      return;
    }
    setMsg(els.setupError, "");
    route();
  });

  function fillProfileForm() {
    els.profileName.value = state.profile.name;
    els.profileLanguage.value = state.profile.language;
    els.profileTone.value = state.profile.tone;
    els.profileLength.value = state.profile.length;
    els.profileExamples.value = state.profile.examples;
  }

  els.profileForm.addEventListener("submit", (e) => {
    e.preventDefault();
    setMsg(els.profileOk, "");
    const err = applyProfileFields(
      els.profileName.value,
      els.profileLanguage.value,
      els.profileTone.value,
      els.profileLength.value,
      els.profileExamples.value
    );
    if (err) {
      setMsg(els.profileError, err);
      return;
    }
    setMsg(els.profileError, "");
    setMsg(els.profileOk, "Profile saved.");
  });

  els.btnMinimize.addEventListener("click", minimizeWidget);
  els.launcher.addEventListener("click", openWidget);
  els.btnSettings.addEventListener("click", () => showSettings("docs"));
  els.closeSettings.addEventListener("click", showChatPane);

  document.querySelectorAll(".stab[data-tab]").forEach((btn) => {
    btn.addEventListener("click", () => showSettings(btn.dataset.tab));
  });

  // Drag floating widget
  (() => {
    let dragging = false;
    let ox = 0;
    let oy = 0;
    els.widgetDrag.addEventListener("pointerdown", (e) => {
      if (e.target.closest("button")) return;
      dragging = true;
      const rect = els.widget.getBoundingClientRect();
      ox = e.clientX - rect.left;
      oy = e.clientY - rect.top;
      els.widget.classList.add("dragging");
      els.widget.setPointerCapture(e.pointerId);
    });
    els.widgetDrag.addEventListener("pointermove", (e) => {
      if (!dragging) return;
      const x = Math.min(window.innerWidth - 80, Math.max(8, e.clientX - ox));
      const y = Math.min(window.innerHeight - 80, Math.max(8, e.clientY - oy));
      els.widget.style.left = `${x}px`;
      els.widget.style.top = `${y}px`;
      els.widget.style.right = "auto";
      els.widget.style.bottom = "auto";
    });
    const endDrag = () => {
      dragging = false;
      els.widget.classList.remove("dragging");
    };
    els.widgetDrag.addEventListener("pointerup", endDrag);
    els.widgetDrag.addEventListener("pointercancel", endDrag);
  })();

  function updateActiveModelLabel() {
    els.activeModelLabel.textContent = state.activeModel
      ? `Model: ${state.activeModel}`
      : "Model: none";
  }

  function renderModels() {
    els.modelList.innerHTML = "";
    CATALOG.forEach((m) => {
      const installed = state.installed.includes(m.id);
      const active = state.activeModel === m.id;
      const li = document.createElement("li");
      li.innerHTML = `<span><strong>${m.id}</strong> · ${m.size}
        <span class="badge ${active ? "active" : installed ? "" : "missing"}">
          ${active ? "active" : installed ? "installed" : "not installed"}
        </span></span>`;
      const actions = document.createElement("div");
      actions.className = "row";
      actions.style.margin = "0";
      if (!installed) {
        const dl = document.createElement("button");
        dl.type = "button";
        dl.className = "secondary";
        dl.textContent = "Download";
        dl.addEventListener("click", () => downloadModel(m.id, dl));
        actions.appendChild(dl);
      } else {
        const act = document.createElement("button");
        act.type = "button";
        act.textContent = active ? "Active" : "Activate";
        act.disabled = active;
        act.addEventListener("click", () => activateModel(m.id));
        actions.appendChild(act);
      }
      li.appendChild(actions);
      els.modelList.appendChild(li);
    });
  }

  async function downloadModel(id, btn) {
    setMsg(els.modelError, "");
    setMsg(els.modelOk, "");
    els.modelStatus.textContent = `Downloading ${id}…`;
    btn.disabled = true;
    await wait(800);
    state.installed.push(id);
    save();
    setMsg(els.modelOk, `Downloaded ${id}.`);
    els.modelStatus.textContent = "Download successful — activate next.";
    renderModels();
  }

  async function activateModel(id) {
    setMsg(els.modelError, "");
    setMsg(els.modelOk, "");
    els.modelStatus.textContent = `Loading ${id}…`;
    await wait(500);
    if (!state.installed.includes(id)) {
      setMsg(els.modelError, "Model unavailable. Choose another.");
      renderModels();
      return;
    }
    state.activeModel = id;
    save();
    updateActiveModelLabel();
    setMsg(els.modelOk, `Active model saved: ${id}`);
    els.modelStatus.textContent = "Return to chat — widget shows active model.";
    renderModels();
  }

  const SUPPORTED = [".txt", ".md", ".text"];
  function isSupported(name) {
    const lower = name.toLowerCase();
    return SUPPORTED.some((ext) => lower.endsWith(ext));
  }

  function renderDocs() {
    els.docList.innerHTML = "";
    if (!state.docs.length) {
      const li = document.createElement("li");
      li.textContent = "No approved documents yet.";
      els.docList.appendChild(li);
      return;
    }
    state.docs.forEach((d) => {
      const li = document.createElement("li");
      li.textContent = `${d.name} · indexed (${d.text.length} chars)`;
      els.docList.appendChild(li);
    });
  }

  els.docBrowse.addEventListener("click", () => els.docInput.click());

  els.docInput.addEventListener("change", async () => {
    setMsg(els.docError, "");
    setMsg(els.docOk, "");
    const files = [...els.docInput.files];
    if (!files.length) {
      els.docChosen.textContent = "No file chosen";
      return;
    }
    els.docChosen.textContent =
      files.length === 1 ? files[0].name : `${files.length} files selected`;
    const bad = files.filter((f) => !isSupported(f.name));
    if (bad.length) {
      setMsg(
        els.docError,
        `Unsupported: ${bad.map((f) => f.name).join(", ")}. Use .txt or .md.`
      );
      els.docInput.value = "";
      return;
    }
    try {
      await wait(400);
      for (const file of files) {
        const text = await file.text();
        if (!text.trim()) throw new Error(`Empty file: ${file.name}`);
        state.docs.push({ name: file.name, text });
      }
      state.indexed = true;
      save();
      renderDocs();
      setMsg(els.docOk, `Indexed ${files.length} file(s).`);
    } catch (err) {
      setMsg(els.docError, `Indexing error: ${err.message}`);
    }
    els.docInput.value = "";
  });

  els.clearDocs.addEventListener("click", () => {
    state.docs = [];
    state.indexed = false;
    save();
    renderDocs();
    setMsg(els.docOk, "");
    setMsg(els.docError, "");
    els.docChosen.textContent = "No file chosen";
  });

  function addMsg(role, text, source) {
    const div = document.createElement("div");
    div.className = `msg ${role}`;
    div.textContent = text;
    if (source) {
      const src = document.createElement("span");
      src.className = "src";
      src.textContent = `Source: ${source}`;
      div.appendChild(src);
    }
    els.chat.appendChild(div);
    els.chat.scrollTop = els.chat.scrollHeight;
  }

  function retrieve(question) {
    const words = question
      .toLowerCase()
      .split(/[^a-z0-9]+/)
      .filter((w) => w.length > 2);
    let best = null;
    let bestScore = 0;
    for (const doc of state.docs) {
      const lower = doc.text.toLowerCase();
      let score = 0;
      for (const w of words) if (lower.includes(w)) score += 1;
      if (score > bestScore) {
        bestScore = score;
        best = doc;
      }
    }
    if (!best || bestScore === 0) return null;
    const hitWord = words.find((w) => best.text.toLowerCase().includes(w)) || "";
    const idx = best.text.toLowerCase().indexOf(hitWord);
    const start = Math.max(0, idx - 40);
    return {
      name: best.name,
      snippet: best.text.slice(start, start + 160).trim(),
    };
  }

  function mockAnswer(question) {
    const hit = retrieve(question);
    const toneOpen =
      state.profile.tone === "Formal"
        ? "Based on your documents:"
        : state.profile.tone === "Concise"
          ? ""
          : "Sure —";
    if (!hit) {
      return {
        text: `${toneOpen} Insufficient document evidence — I will not invent a source.`,
        source: null,
      };
    }
    return {
      text: `${toneOpen} ${hit.snippet}${hit.snippet.length >= 160 ? "…" : ""}`,
      source: hit.name,
    };
  }

  els.chatForm.addEventListener("submit", async (e) => {
    e.preventDefault();
    const q = els.userInput.value.trim();
    if (!q) return;
    if (!state.activeModel) {
      els.chatStatus.textContent = "Open ⚙ → Model and activate one first.";
      return;
    }
    if (!state.docs.length) {
      els.chatStatus.textContent = "Open ⚙ → Documents and upload notes first.";
      return;
    }
    addMsg("user", q);
    els.userInput.value = "";
    els.chatStatus.textContent = "Retrieving…";
    await wait(280);
    const { text, source } = mockAnswer(q);
    addMsg("bot", text, source);
    els.chatStatus.textContent = "Ready for a follow-up.";
  });

  function folderAccessible(path) {
    return /^([a-zA-Z]:\\|\\\\|\/)/.test((path || "").trim());
  }

  function renderSchedule() {
    els.notesFolder.value = state.schedule.folder;
    els.summaryFreq.value = state.schedule.frequency;
    els.sumStatus.textContent = state.schedule.enabled
      ? `Active · ${state.schedule.frequency} · ${state.schedule.folder}`
      : "Schedule disabled.";
  }

  els.enableSchedule.addEventListener("click", () => {
    setMsg(els.sumError, "");
    setMsg(els.sumOk, "");
    const folder = els.notesFolder.value.trim();
    if (!folderAccessible(folder)) {
      setMsg(els.sumError, "Folder missing / inaccessible. Enter a valid path.");
      return;
    }
    state.schedule = {
      enabled: true,
      folder,
      frequency: els.summaryFreq.value,
    };
    save();
    renderSchedule();
    setMsg(els.sumOk, "Schedule enabled.");
  });

  els.runOnce.addEventListener("click", async () => {
    setMsg(els.sumError, "");
    setMsg(els.sumOk, "");
    if (!state.schedule.enabled) {
      setMsg(els.sumError, "Enable the schedule first.");
      return;
    }
    await wait(400);
    if (!state.docs.length) {
      els.sumOutput.textContent = "Log: no changes; wait for next run.";
      return;
    }
    const topics = state.docs.map((d) => d.name).join(", ");
    els.sumOutput.textContent = `Summary OK.\nSources: ${topics}\n${state.docs
      .map((d) => `• ${d.name}: ${d.text.slice(0, 70)}…`)
      .join("\n")}`;
    setMsg(els.sumOk, "Summary ready.");
  });

  els.disableSchedule.addEventListener("click", () => {
    state.schedule.enabled = false;
    save();
    renderSchedule();
    setMsg(els.sumOk, "");
    els.sumStatus.textContent = "Schedule disabled.";
  });

  /** Guided dynamic demo of everyday float usage */
  async function playWalkthrough() {
    if (demoRunning) return;
    demoRunning = true;
    els.playDemo.disabled = true;
    clearTimeout(demoTimer);

    // Ensure profile
    if (!state.profileSet) {
      els.demoCaption.textContent = "Step 1 · One-time profile setup";
      els.setupName.value = "Alex";
      els.setupTone.value = "Friendly";
      els.setupLength.value = "Medium";
      els.setupExamples.value = "Prefer short practical answers";
      await wait(900);
      applyProfileFields(
        els.setupName.value,
        "English",
        "Friendly",
        "Medium",
        els.setupExamples.value
      );
      els.chat.innerHTML = "";
      delete els.chat.dataset.booted;
      route();
    } else {
      openWidget();
    }

    els.demoCaption.textContent = "Step 2 · Occasional setup: activate model";
    showSettings("model");
    await wait(700);
    if (!state.activeModel) {
      await activateModel(state.installed[0] || "qwen3.5:9b");
    }
    await wait(600);

    els.demoCaption.textContent = "Step 3 · Occasional setup: index a notes file";
    showSettings("docs");
    if (!state.docs.some((d) => d.name === DEMO_NOTE.name)) {
      state.docs.push({ ...DEMO_NOTE });
      state.indexed = true;
      save();
      renderDocs();
      setMsg(els.docOk, "Indexed CS180.md (demo).");
    }
    await wait(900);

    els.demoCaption.textContent = "Step 4 · Everyday use: float chat over the desktop";
    showChatPane();
    await wait(500);
    const q = "How does the floating widget help while I work?";
    addMsg("user", q);
    els.chatStatus.textContent = "Retrieving…";
    await wait(450);
    const { text, source } = mockAnswer(q);
    addMsg("bot", text, source);
    els.chatStatus.textContent = "Ready for a follow-up.";
    await wait(1200);

    els.demoCaption.textContent = "Step 5 · Minimize to bubble — stays out of the way";
    minimizeWidget();
    await wait(1100);

    els.demoCaption.textContent = "Step 6 · Re-open float for the next quick question";
    openWidget();
    await wait(500);
    const q2 = "What controls tone and length?";
    addMsg("user", q2);
    await wait(400);
    const a2 = mockAnswer(q2);
    addMsg("bot", a2.text, a2.source);

    els.demoCaption.textContent =
      "Done · Daily work = float chat; settings only for profile / files / model.";
    demoRunning = false;
    els.playDemo.disabled = false;
  }

  els.playDemo.addEventListener("click", playWalkthrough);

  els.resetDemo.addEventListener("click", () => {
    clearTimeout(demoTimer);
    demoRunning = false;
    els.playDemo.disabled = false;
    localStorage.removeItem(STORAGE_KEY);
    location.reload();
  });

  tickClock();
  setInterval(tickClock, 30_000);
  load();
  route();
})();
