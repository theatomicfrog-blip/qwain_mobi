import re

with open('cpp_source/qslpreviewdialog.cpp', 'r') as f:
    text = f.read()

target = """    if (m_printQueue.size() > 1 && m_batchSequence == BatchSequence::CardsThenEnvelopes) {
        // We are printing all cards, then all envelopes
        if (!m_batchPrintingEnvelopesPhase && printCard) {
            m_previewingEnvelope = false;
            if (m_templateList->currentItem()) {
                onTemplateChosen(m_templateList->currentItem()->text());
            } else if (m_templateList->count() > 0) {
                onTemplateChosen(m_templateList->item(0)->text());
            }
            if (!performPrint(false)) {
                m_previewingEnvelope = origPreviewEnv;
                return;
            }
        } else if (m_batchPrintingEnvelopesPhase && printEnvelope) {
            m_previewingEnvelope = true;
            if (m_envTemplateList->currentItem()) {
                onTemplateChosen(m_envTemplateList->currentItem()->text());
            } else if (m_envTemplateList->count() > 0) {
                onTemplateChosen(m_envTemplateList->item(0)->text());
            }
            if (!performPrint(true)) {
                m_previewingEnvelope = origPreviewEnv;
                return;
            }
        }
    } else {
        // Normal or CardEnvelope sequence
        if (printCard) {
            m_previewingEnvelope = false;
            if (m_templateList->currentItem()) {
                onTemplateChosen(m_templateList->currentItem()->text());
            } else if (m_templateList->count() > 0) {
                onTemplateChosen(m_templateList->item(0)->text());
            }
            if (!performPrint(false)) {
                m_previewingEnvelope = origPreviewEnv;
                return;
            }
        }
        
        if (printEnvelope) {
            m_previewingEnvelope = true;
            if (m_envTemplateList->currentItem()) {
                onTemplateChosen(m_envTemplateList->currentItem()->text());
            } else if (m_envTemplateList->count() > 0) {
                onTemplateChosen(m_envTemplateList->item(0)->text());
            }
            if (!performPrint(true)) {
                m_previewingEnvelope = origPreviewEnv;
                return;
            }
        }
    }"""

replacement = """    bool keepGoing = true;
    while(keepGoing) {
        keepGoing = false;
        
        if (m_printQueue.size() > 1 && m_batchSequence == BatchSequence::CardsThenEnvelopes) {
            // We are printing all cards, then all envelopes
            if (!m_batchPrintingEnvelopesPhase && printCard) {
                m_previewingEnvelope = false;
                if (m_templateList->currentItem()) {
                    onTemplateChosen(m_templateList->currentItem()->text());
                } else if (m_templateList->count() > 0) {
                    onTemplateChosen(m_templateList->item(0)->text());
                }
                if (!performPrint(false)) {
                    m_previewingEnvelope = origPreviewEnv;
                    return;
                }
            } else if (m_batchPrintingEnvelopesPhase && printEnvelope) {
                m_previewingEnvelope = true;
                if (m_envTemplateList->currentItem()) {
                    onTemplateChosen(m_envTemplateList->currentItem()->text());
                } else if (m_envTemplateList->count() > 0) {
                    onTemplateChosen(m_envTemplateList->item(0)->text());
                }
                if (!performPrint(true)) {
                    if (m_reprintCardRequested) {
                        m_previewingEnvelope = false;
                        if (m_templateList->currentItem()) {
                            onTemplateChosen(m_templateList->currentItem()->text());
                        } else if (m_templateList->count() > 0) {
                            onTemplateChosen(m_templateList->item(0)->text());
                        }
                        if (performPrint(false)) {
                            keepGoing = true; // Try printing the envelope again
                        }
                    }
                    if (!keepGoing) {
                        m_previewingEnvelope = origPreviewEnv;
                        return;
                    }
                }
            }
        } else {
            // Normal or CardEnvelope sequence
            if (printCard) {
                m_previewingEnvelope = false;
                if (m_templateList->currentItem()) {
                    onTemplateChosen(m_templateList->currentItem()->text());
                } else if (m_templateList->count() > 0) {
                    onTemplateChosen(m_templateList->item(0)->text());
                }
                if (!performPrint(false)) {
                    m_previewingEnvelope = origPreviewEnv;
                    return;
                }
            }
            
            if (printEnvelope) {
                m_previewingEnvelope = true;
                if (m_envTemplateList->currentItem()) {
                    onTemplateChosen(m_envTemplateList->currentItem()->text());
                } else if (m_envTemplateList->count() > 0) {
                    onTemplateChosen(m_envTemplateList->item(0)->text());
                }
                if (!performPrint(true)) {
                    if (m_reprintCardRequested) {
                        printCard = true;
                        keepGoing = true;
                    }
                    if (!keepGoing) {
                        m_previewingEnvelope = origPreviewEnv;
                        return;
                    }
                }
            }
        }
    }"""

if target in text:
    print("Found target!")
    with open('cpp_source/qslpreviewdialog.cpp', 'w') as f:
        f.write(text.replace(target, replacement))
else:
    print("Target not found.")
