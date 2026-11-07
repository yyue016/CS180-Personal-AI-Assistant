document.addEventListener("DOMContentLoaded", function () {
    const input = document.getElementById("messageInput");
    const button = document.getElementById("sendButton");
    const chat = document.getElementById("chatMessages");

    if (!input || !button || !chat) return;

    function sendMessage() {
        const message = input.value.trim();

        if (!message) return;

        const wrapper = document.createElement("div");
        wrapper.className = "d-flex justify-content-end";

        const bubble = document.createElement("div");
        bubble.className = "alert alert-primary";
        bubble.style.overflowWrap = "anywhere";
        bubble.textContent = message;

        wrapper.appendChild(bubble);
        chat.appendChild(wrapper);

        input.value = "";
        chat.scrollTop = chat.scrollHeight;
        input.focus();
    }

    button.addEventListener("click", sendMessage);

    input.addEventListener("keydown", function (event) {
        if (event.key === "Enter") {
            event.preventDefault();
            sendMessage();
        }
    });
});
