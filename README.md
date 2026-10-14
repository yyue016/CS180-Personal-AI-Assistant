# CS180 -- Personal AI Assistant

## Project Overview

General-purpose AI assistants often require users to repeat their preferences and upload the same documents for different tasks. Meanwhile, personal knowledge may be scattered across local notes and files.

Personal AI Assistant aims to bring that knowledge and those preferences into a single workflow. The proposed system will combine an existing open-source large language model (LLM), retrieval-augmented generation (RAG), and an editable user profile. RAG will provide relevant personal knowledge at request time, while the profile will guide language, tone, and level of detail. Controlled agent execution will support tasks such as recurring summaries.

## Goals

- Answer questions using an approved personal document collection.
- Reduce repeated preference-setting through a saved, editable profile.
- Help users turn information into useful outputs, such as study materials and recurring summaries.

## Planned Features

| Feature | Proposed behavior |
| --- | --- |
| LLM selection | Let users choose an available LLM and save their selection. Compatible packages and a model will be selected after an early benchmark. |
| Personal profile | Collect initial profile information through personality questions and uploaded personal profile information; allow users to edit response language, tone, length, and example preferences. |
| Personal document Q&A | Parse approved files, retrieve relevant passages, and use them to inform answers with source references. |
| File and task controls (If Time permits)| Let users manage file access, inspect task logs, cancel tasks, and disable scheduled jobs. |
| Recurring summaries (If Time permits)| Read new or changed files in a selected notes folder on a schedule and produce summaries with source references and suggested review topics. |

## Proposed Development Timeline

| Period | Planned work |
| --- | --- |
| Weeks 1-2 | Confirm scope and team roles. |
| Weeks 3-4 | Benchmark a model; build file ingestion and chat. |
| Weeks 5-6 | Test model performance in the local chat interface and evaluate model selection. |
| Weeks 7-8 | Implement retrieval, source references, and the editable personal profile. |
| Weeks 8-9 | Evaluate the prototype, resolve integration issues, and prepare the final demonstration. |

## Evaluation Targets

The following are **planned evaluations and targets, not measured results**.

| Area | Proposed evaluation | Target or reporting expectation |
| --- | --- | --- |
| Document grounding | Use 10 document questions with expected source passages. | At least 80% correct grounding. |
| Personalization | Use 10 paired prompts to check adherence to saved style preferences. | Check whether responses follow saved preferences; no numerical threshold is specified. |
| Responsiveness | Record response latency. | Report latency; no numerical threshold is specified. |
| Unsupported questions | Test questions unsupported by the personal corpus. | Examine behavior; no numerical acceptance criterion is specified. |
| Optional fine-tuning | Compare a small adapter with the RAG and profile baseline on held-out tasks, if resources permit. | Report the comparison if the extension is pursued. |

## Team

**Team name:** team pAa

**Members:** Hao Zhang, Kunhang Li, Oliver Ren, Yang Yue.

## Current Responsibilities

| Team Members | Responsibilities |
| --- | --- |
| Hao Zhang & Yang Yue | Frontend design and development |
| Oliver Ren & Kunhang Li | Backend development and model testing |
